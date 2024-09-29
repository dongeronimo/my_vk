#include "app/window.h"
#include "vk/instance.h"
#include "vk/device.h"
#include "vk/debug_utils.h"
#include "vk/swap_chain.h"
#include "vk/descriptor_service.h"
#include "vk/samplers_service.h"
#include "vk/image_service.h"
#include "vk/render_pass.h"
#include "utils/vk_utils.h"
#include "vk/framebuffer.h"
#include "vk/mesh_service.h"
#include "vk/synchronization_service.h"
#include "entities/pipeline.h"
#include "entities/game_object.h"
#include <chrono>
#include "app/timer.h"
#include <entities/camera_uniform_buffer.h>
#include "entities/frame.h"
#include "entities/light_uniform_buffer.h"
#include <algorithm>

typedef hash_t renderpass_hash_t;
typedef hash_t pipeline_hash_t;


std::vector<entities::GameObject*> gObjects{};
std::map<pipeline_hash_t, entities::Pipeline*> gPipelines;
/// <summary>
/// one renderpass mau have 0..n pipelines
/// </summary>
std::map<renderpass_hash_t, std::vector<pipeline_hash_t>> gRenderPassPipelineTable;
/// <summary>
/// Holds the render passes, on the order they are to be run
/// </summary>
std::vector<vk::RenderPass*> gOrderedRenderpasses;
entities::Pipeline* CreateDemoPipeline(vk::RenderPass* renderPass, vk::Device& device, vk::DescriptorService& descriptorService);
entities::Pipeline* CreatePhongSolidColorPipeline(vk::RenderPass* renderPass, vk::Device& device, vk::DescriptorService& descriptorService,entities::TLightCallback lightCallback);

std::map<pipeline_hash_t, std::vector<entities::GameObject*>> gPipelineGameObjectTable;
int main(int argc, char** argv)
{
    glfwInit();
    app::Window mainWindow(SCREEN_WIDTH, SCREEN_HEIGH);
    //Create the instance
    vk::Instance instance(mainWindow.GetWindow());
    instance.ChoosePhysicalDevice(VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU, vk::YES);
    //Create the device
    vk::Device device(instance.GetPhysicalDevice(),instance.GetInstance(), instance.GetSurface(), vk::GetValidationLayerNames());
    //Create the swap chain
    vk::SwapChain swapChain;
    //create the render pass for onscreen rendering
    //TODO depth buffer - remeber that we have a version that uses depth buffer.
    vk::RenderPass* mainRenderPass = new vk::RenderPass( swapChain.GetFormat(), "mainRenderPass");
    gOrderedRenderpasses.push_back(mainRenderPass);
    //create the framebuffer for onscreen rendering
    //TODO depth buffer - no depth buffer, need to add one
    vk::Framebuffer mainFramebuffer(swapChain.GetImageViews(), swapChain.GetExtent(), *mainRenderPass, "mainFramebuffer");
    //create the samplers
    vk::SamplerService samplersService;
    //create the images
    vk::ImageService imageService({"blackBrick.png","brick.png","floor01.jpg"});
    //create the descriptor infrastructure: descriptorSetLayouts, DescriptorPools and DescriptorSets
    vk::DescriptorService descriptorService(samplersService,
        imageService);
    //load meshes
    vk::MeshService meshService({ "monkey.glb" });
    //create the pipelines
    entities::Pipeline* demoPipeline = CreateDemoPipeline(mainRenderPass, device, descriptorService);
    entities::TLightCallback lightCallback = []() {
        entities::LightBuffers lights;
        memset(&lights, 0, sizeof(entities::LightBuffers));
        lights.ambient.colorAndIntensity = { 1,1,1,0.1f };
        lights.directionalLights.colorAndIntensity[0] = { 1,1,1,1 };
        lights.directionalLights.direction[0] = { 1,0,0 };
        return lights;
    };
    entities::Pipeline* phongSolidColor = CreatePhongSolidColorPipeline(mainRenderPass, device, descriptorService, lightCallback);  

    gPipelines.insert({ utils::Hash("phongSolidColor") , phongSolidColor });
    gPipelines.insert({ utils::Hash("demoPipeline"), demoPipeline });
    //register that mainRenderPass has these pipelines
    gRenderPassPipelineTable.insert({ utils::Hash("mainRenderPass"), {utils::Hash("phongSolidColor"),  utils::Hash("demoPipeline")} });
    //create the synchronization objects
    vk::SyncronizationService syncService;
    //Create a game object
    auto gFoo = new entities::GameObject("foo", descriptorService,"demoPipeline", meshService.GetMesh("monkey.glb"));
    gFoo->SetPosition(glm::vec3( 0,0,0 ));
    gFoo->SetOrientation(glm::quat());
    gFoo->OnDraw = [](entities::GameObject& go, entities::Pipeline& pipeline, uint32_t currentFrame, VkCommandBuffer commandBuffer) {
        if (pipeline.Hash() == utils::Hash("phongSolidColor")) {
            entities::ColorPushConstantData color{ {0,1,0,1} };
            pipeline.SetPushConstant<entities::ColorPushConstantData>(
                color, go.mId, commandBuffer, VK_SHADER_STAGE_VERTEX_BIT
            );
        }
        };
    gObjects.push_back(gFoo);
    auto gBar = new entities::GameObject("bar", descriptorService, "demoPipeline", meshService.GetMesh("monkey.glb"));
    gBar->SetPosition(glm::vec3(2, 0, 0));
    gBar->SetOrientation(glm::quat());

    gObjects.push_back(gBar);
    //add the game objects to their pipelines
    gPipelineGameObjectTable.insert({ phongSolidColor->Hash(), {gFoo}});
    gPipelineGameObjectTable.insert({ demoPipeline->Hash(), {gBar}});
    //Define the camera
    entities::CameraUniformBuffer cameraBuffer;
    cameraBuffer.cameraPos = glm::vec3(5.0f, 5.0f, 5.0f);
    cameraBuffer.view = glm::lookAt(cameraBuffer.cameraPos, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    //some perspective projection
    cameraBuffer.proj = glm::perspective(glm::radians(45.0f),
        swapChain.GetExtent().width / (float)swapChain.GetExtent().height, 0.1f, 100.0f);
    //GOTCHA: GLM is for opengl, the y coords are inverted. With this trick we the correct that
    cameraBuffer.proj[1][1] *= -1;
    //define the main loop callback
    app::Timer timer;
    size_t currentFrame = 0;
    uint32_t imageIndex = UINT32_MAX;
    std::vector<VkCommandBuffer> commandBuffers = device.CreateCommandBuffer("mainCommandBuffers", MAX_FRAMES_IN_FLIGHT);


    mainWindow.OnRender = [&timer, &syncService, &currentFrame, 
        &swapChain, &mainFramebuffer, &descriptorService, &cameraBuffer]
        (app::Window* w) 
        {
            timer.Advance();//advance the clock
            entities::Frame frame(currentFrame, syncService, swapChain);
            frame.BeginFrame();
            for (auto R : gOrderedRenderpasses) {
                //for each render pass R begin R
                R->BeginRenderPass({ 1,1,1,1 }, { 1.0f, 0 },
                    mainFramebuffer.GetFramebuffer(frame.ImageIndex()), { SCREEN_WIDTH,SCREEN_HEIGH },
                    frame.CommandBuffer());
                //for each pipeline P of R bind P
                std::vector<pipeline_hash_t> pipelineHashes = gRenderPassPipelineTable.at(utils::Hash(R->mName));
                std::vector<entities::Pipeline*> pipelines(pipelineHashes.size());
                std::transform(pipelineHashes.begin(), pipelineHashes.end(), pipelines.begin(), [](auto h) {
                    return gPipelines.at(h);
                });
                for (auto P : pipelines) {
                    // Bind the pipeline
                    P->Bind(frame.CommandBuffer(), frame.mCurrentFrame);
                    //set the camera data for the main render pass, all objects will use the same camera
                    std::vector<uintptr_t> cameraDescriptorSetsAddrs = descriptorService.DescriptorSetsBuffersAddrs(vk::CAMERA_LAYOUT_NAME, 0);
                    void* cameraDescriptorSetAddr = reinterpret_cast<void*>(cameraDescriptorSetsAddrs[currentFrame]);
                    memcpy(cameraDescriptorSetAddr, &cameraBuffer, sizeof(entities::CameraUniformBuffer));
                    //for each gameobject G that uses P draw G
                    if (gPipelineGameObjectTable.count(P->Hash())>0)
                    {
                        std::vector<entities::GameObject*> gameObjects = gPipelineGameObjectTable.at(P->Hash());
                        for (auto G : gameObjects) {
                            //Draw the object
                            G->Draw(frame.CommandBuffer(), *P, currentFrame);
                        }
                    }
                }
                //finish R
                R->EndRenderPass(frame.CommandBuffer());
            }
            frame.EndFrame();
            currentFrame = currentFrame + 1;
            currentFrame = currentFrame % MAX_FRAMES_IN_FLIGHT;
    };
    ///begin the main loop - blocks here
    mainWindow.MainLoop();
    return 0;
}


entities::Pipeline* CreateDemoPipeline(vk::RenderPass* renderPass, vk::Device& device, vk::DescriptorService& descriptorService)
{
    //TODO phong: add depth
    entities::Pipeline* demoPipeline = (new entities::PipelineBuilder("demoPipeline", descriptorService))->
        SetRenderPass(renderPass)->
        SetShaderModules(
            entities::LoadShaderModule(device.GetDevice(), "demo.vert.spv"),
            entities::LoadShaderModule(device.GetDevice(), "demo.frag.spv")
        )->
        SetDescriptorSetLayouts({
            descriptorService.DescriptorSetLayout(vk::CAMERA_LAYOUT_NAME),
            descriptorService.DescriptorSetLayout(vk::MODEL_MATRIX_LAYOUT_NAME) })->
            SetVertexInputStateInfo(entities::GetVertexInputInfoForMesh())->
        SetRasterizerStateInfo(entities::GetBackfaceCullClockwiseRasterizationInfo())->
        SetDepthStencilStateInfo(entities::GetDefaultDepthStencil())->
        SetColorBlending(entities::GetNoColorBlend())->
        SetViewport(entities::GetViewportForSize(SCREEN_WIDTH, SCREEN_HEIGH), entities::GetScissor(SCREEN_WIDTH, SCREEN_HEIGH))-> //TODO resize: hardcoded screen size
        Build();
    return demoPipeline;
}

entities::Pipeline* CreatePhongSolidColorPipeline(vk::RenderPass* renderPass, vk::Device& device, vk::DescriptorService& descriptorService,
    entities::TLightCallback lightCallback) {
    //TODO phong: add depth
    entities::Pipeline* phongSolidColor = (new entities::PipelineBuilder("phongSolidColor", descriptorService))->
        SetPushConstantRanges({ entities::GetPushConstantRangeFor<entities::ColorPushConstantData>(VK_SHADER_STAGE_VERTEX_BIT) })->
        SetRenderPass(renderPass)->
        SetShaderModules(
            entities::LoadShaderModule(device.GetDevice(), "phong_color.vert.spv"),
            entities::LoadShaderModule(device.GetDevice(), "phong_color.frag.spv")
        )->
        SetDescriptorSetLayouts({ //TODO phong: add per-frag lightining data
            descriptorService.DescriptorSetLayout(vk::CAMERA_LAYOUT_NAME),
            descriptorService.DescriptorSetLayout(vk::MODEL_MATRIX_LAYOUT_NAME),
            descriptorService.DescriptorSetLayout(vk::LIGHTNING_LAYOUT_NAME)})->
            SetVertexInputStateInfo(entities::GetVertexInputInfoForMesh())->
        SetRasterizerStateInfo(entities::GetBackfaceCullClockwiseRasterizationInfo())->
        SetDepthStencilStateInfo(entities::GetDefaultDepthStencil())->
        SetColorBlending(entities::GetNoColorBlend())->
        SetViewport(entities::GetViewportForSize(SCREEN_WIDTH, SCREEN_HEIGH), entities::GetScissor(SCREEN_WIDTH, SCREEN_HEIGH))-> //TODO resize: hardcoded screen size
        SetLightDataCallback(lightCallback)->
        Build();
    return phongSolidColor;
}
