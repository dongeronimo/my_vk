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
#include <vk\depth_buffer.h>
#include "utils/concatenate.h"
#include <glm/gtc/matrix_transform.hpp>
#include <utils/ring_buffer.h>
float orthoSize = 100.0f;
typedef hash_t renderpass_hash_t;
typedef hash_t pipeline_hash_t;
const uint32_t SHADOW_MAP_WIDTH = 512;
const uint32_t SHADOW_MAP_HEIGHT = 512;
//std::vector<vk::DepthBuffer*> gDepthBuffersForShadowMapping(MAX_FRAMES_IN_FLIGHT);
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
entities::LightBuffers lights;
entities::CameraUniformBuffer cameraBuffer;
entities::Pipeline* CreateDemoPipeline(vk::RenderPass* renderPass, vk::Device& device, vk::DescriptorService& descriptorService);
entities::Pipeline* CreatePhongSolidColorPipeline(vk::RenderPass* renderPass, vk::Device& device, vk::DescriptorService& descriptorService,entities::TLightCallback lightCallback);
entities::Pipeline* CreateShadowMapPipeline(vk::RenderPass* renderPass, vk::Device& device, vk::DescriptorService& descriptorService, uint32_t w, uint32_t h);
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
    //a fake shadow render pass to study how to pass data from one render pass to another.
    vk::RenderPass* fakeShadowsRenderPass = vk::RenderPass::FakeShadowMapPass("FakeShadowMapRenderPass");
    
    /////////Create the framebuffer for the fake shadow map
    ///TODO refactor: this is not the correct place for those things. They are here for debugging
    std::vector<VkImage> fakeShadowMapImages(MAX_FRAMES_IN_FLIGHT);
    std::vector<VkImageView> fakeShadowMapImageViews(MAX_FRAMES_IN_FLIGHT);
    std::vector<VkDeviceMemory> fakeShadowMapImageMemories(MAX_FRAMES_IN_FLIGHT);
    VkImage fakeShadowMapDepthImage;
    VkImageView fakeShadowMapDepthImageView;
    VkDeviceMemory fakeShadowMapDeviceMemory;
    vk::ImageService::CreateImage(SHADOW_MAP_WIDTH, SHADOW_MAP_HEIGHT, VK_FORMAT_D32_SFLOAT, 
        VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        fakeShadowMapDepthImage, fakeShadowMapDeviceMemory);
    fakeShadowMapDepthImageView = vk::ImageService::CreateImageView(fakeShadowMapDepthImage, VK_FORMAT_D32_SFLOAT, VK_IMAGE_ASPECT_DEPTH_BIT);

    for (auto i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vk::ImageService::CreateImage(SHADOW_MAP_WIDTH, SHADOW_MAP_HEIGHT, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, fakeShadowMapImages[i], fakeShadowMapImageMemories[i]);
        fakeShadowMapImageViews[i] = vk::ImageService::CreateImageView(fakeShadowMapImages[i], VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);
        auto n0 = Concatenate("FakeShadowMapRenderPassImage", i);
        auto n1 = Concatenate("FakeShadowMapRenderPassImageView", i);
        auto n2 = Concatenate("FakeShadowMapRenderPassImageMemory", i);
        SET_NAME(fakeShadowMapImages[i], VK_OBJECT_TYPE_IMAGE, n0.c_str());
        SET_NAME(fakeShadowMapImageViews[i], VK_OBJECT_TYPE_IMAGE_VIEW, n1.c_str());
        SET_NAME(fakeShadowMapImageMemories[i], VK_OBJECT_TYPE_DEVICE_MEMORY, n2.c_str());
    }

    //create the render pass for onscreen rendering
    vk::RenderPass* mainRenderPass = new vk::RenderPass(utils::FindDepthFormat(instance.GetPhysicalDevice()), swapChain.GetFormat(),"mainRenderPass");
    mainRenderPass->mOnRenderPassEndCallback = [&fakeShadowMapImages, &fakeShadowMapDepthImage](vk::RenderPass* renderPass, VkCommandBuffer cmdBuffer, uint32_t currentFrame) {
        //move the color attachment from the texture to output of pipeline
        utils::TransitionImageLayout(
            cmdBuffer,
            fakeShadowMapImages[currentFrame],
            VK_FORMAT_R8G8B8A8_UNORM,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            VK_IMAGE_ASPECT_COLOR_BIT,
            VK_ACCESS_SHADER_READ_BIT,
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
        utils::TransitionImageLayout(
            cmdBuffer,
            fakeShadowMapDepthImage,
            VK_FORMAT_D32_SFLOAT,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,//VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            VK_IMAGE_ASPECT_DEPTH_BIT,
            VK_ACCESS_SHADER_READ_BIT,
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT
        );
    };
    //
    gOrderedRenderpasses.push_back(fakeShadowsRenderPass);
    gOrderedRenderpasses.push_back(mainRenderPass);
    //create the framebuffer for onscreen rendering
    ///Creates the depth buffers for the main framebuffer, one per frame
    vk::DepthBuffer depthBuffer(swapChain.GetExtent().width, swapChain.GetExtent().height);
    VkImageView depthBufferImageView = depthBuffer.GetImageView();
    vk::Framebuffer mainFramebuffer(swapChain.GetImageViews(), depthBufferImageView,
        swapChain.GetExtent(), *mainRenderPass, "mainFramebuffer");
    /////////////fake shadows
    vk::Framebuffer fakeShadowMapFramebuffer(fakeShadowMapImageViews, fakeShadowMapDepthImageView, { SHADOW_MAP_WIDTH, SHADOW_MAP_HEIGHT }, *fakeShadowsRenderPass, "FakeShadowMapFramebuffer");
    fakeShadowsRenderPass->mOnRenderPassEndCallback = [&fakeShadowMapImages, &fakeShadowMapDepthImage](vk::RenderPass* rp, VkCommandBuffer cmdBuffer, uint32_t currentFrame) {
        //move the color attachment from the layout that receives the render result to the layout
        //that can be sampled by shaders
        utils::TransitionImageLayout(
            cmdBuffer,
            fakeShadowMapImages[currentFrame], 
            VK_FORMAT_R8G8B8A8_UNORM, 
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_IMAGE_ASPECT_COLOR_BIT,
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            VK_ACCESS_SHADER_READ_BIT,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
        utils::TransitionImageLayout(
            cmdBuffer,
            fakeShadowMapDepthImage,
            VK_FORMAT_D32_SFLOAT,
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_IMAGE_ASPECT_DEPTH_BIT,
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            VK_ACCESS_SHADER_READ_BIT,
            VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
        );
    };
 
    //create the samplers
    vk::SamplerService samplersService;
    //create the images
    vk::ImageService imageService({"blackBrick.png","brick.png","floor01.jpg"});
    //create the descriptor infrastructure: descriptorSetLayouts, DescriptorPools and DescriptorSets
    vk::DescriptorService descriptorService(samplersService,
        imageService, fakeShadowMapImageViews, fakeShadowMapDepthImageView);
    //load meshes
    vk::MeshService meshService({ "monkey.glb", "4x4tile.glb", "box.glb"});
    //create the pipelines
    entities::Pipeline* demoPipeline = CreateDemoPipeline(mainRenderPass, device, descriptorService);
    entities::TLightCallback lightCallback = []() {
        lights.ambient.colorAndIntensity = { 1,1,1,0.01f };
        lights.directionalLights.diffuseColorAndIntensity = { 1,1,1,1 };
        lights.directionalLights.direction = { 0, -1, -1 };
        ////light matrix calc
        glm::mat4 projection = glm::ortho(
            -orthoSize, orthoSize, -orthoSize, orthoSize, 1.0f, 200.f);
        //GOTCHA: GLM is for opengl, the y coords are inverted. With this trick we the correct that
        projection[1][1] *= -1;
        glm::vec3 lightDirection = glm::normalize(lights.directionalLights.direction);
        glm::vec3 originInInf = glm::vec3(0, 0, 100.0f);
        glm::mat4 viewMatrix = glm::lookAt(originInInf,
            originInInf + lightDirection,
            glm::vec3(0, 0, 1));
        glm::mat4 lightMatrix = projection * viewMatrix;
        lights.directionalLights.lightMatrix = lightMatrix;
        return lights;
    };
    entities::Pipeline* phongSolidColor = CreatePhongSolidColorPipeline(mainRenderPass, device, descriptorService, lightCallback);  
    entities::Pipeline* fakeShadowMapPipeline = CreateShadowMapPipeline(
        fakeShadowsRenderPass, device, descriptorService, SHADOW_MAP_WIDTH, SHADOW_MAP_HEIGHT);
    gPipelines.insert({ utils::Hash("phongSolidColor") , phongSolidColor });
    gPipelines.insert({ utils::Hash("demoPipeline"), demoPipeline });
    gPipelines.insert({ utils::Hash("fakeShadowMapPipeline"), fakeShadowMapPipeline });
    //register that mainRenderPass has these pipelines
    gRenderPassPipelineTable.insert({ utils::Hash("mainRenderPass"), {
        utils::Hash("phongSolidColor") 
        /*,utils::Hash("demoPipeline")*/
        } });
    gRenderPassPipelineTable.insert({ utils::Hash("FakeShadowMapRenderPass"), {utils::Hash("fakeShadowMapPipeline")} });
    //create the synchronization objects
    vk::SyncronizationService syncService;
    //////////////////////GAME OBJECTS//////////////////////
    //Create a game object
    auto gFoo = new entities::GameObject("foo", descriptorService, meshService.GetMesh("monkey.glb"));
    gFoo->SetPosition(glm::vec3( 0,4,0 ));
    gFoo->SetOrientation(glm::quat());
    gFoo->OnDraw = [&descriptorService](entities::GameObject& go, entities::Pipeline& pipeline, uint32_t currentFrame, VkCommandBuffer commandBuffer) {
        if (pipeline.Hash() == utils::Hash("phongSolidColor")) { //a very rudimentary material system, passing the color that i want.
            entities::ColorPushConstantData color{ {0,1,0,1} };
            pipeline.SetPushConstant<entities::ColorPushConstantData>(
                color, go.mId, commandBuffer, VK_SHADER_STAGE_VERTEX_BIT
            );
            ///use the shadow map samplers
            std::vector<VkDescriptorSet> samplerDescriptorSetRingBuffer = descriptorService.DescriptorSet(vk::FAKE_SHADOW_MAP_SAMPLERS);
            vkCmdBindDescriptorSets(
                commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                pipeline.PipelineLayout(),
                vk::FAKE_SHADOW_MAP_OUTPUT_SAMPLERS_SET,
                1,
                &samplerDescriptorSetRingBuffer[currentFrame],
                0,
                nullptr
            );
        }
        };
    gObjects.push_back(gFoo);
    auto gBar = new entities::GameObject("bar", descriptorService, meshService.GetMesh("monkey.glb"));
    gBar->SetPosition(glm::vec3(2, 4, 0));
    gBar->SetOrientation(glm::quat());
    gObjects.push_back(gBar);
    std::vector<entities::GameObject*> tiles;
    //let's create some tiles
    for (auto i = 0; i < 5; i++) {
        for (auto j = 0; j < 5; j++) {
            auto name = Concatenate("Tile[", i, ",", j, "]");
            auto tile = new entities::GameObject(name, descriptorService, meshService.GetMesh("4x4tile.glb"));
            tile->SetPosition(glm::vec3{ i * 2, j * 2, 0.0f }+ glm::vec3{0, 0, -4.0f});
            tile->SetOrientation(glm::quat());
            tile->OnDraw = [i, j, &descriptorService](entities::GameObject& go, entities::Pipeline& pipeline, uint32_t currentFrame, VkCommandBuffer commandBuffer) {
                if (pipeline.Hash() == utils::Hash("phongSolidColor")) {
                    glm::vec4 _color = { (float)i / 5.0f, (float)j / 5.0f, 0.24f, 1 };
                    entities::ColorPushConstantData color{ _color };
                    pipeline.SetPushConstant<entities::ColorPushConstantData>(
                        color, go.mId, commandBuffer, VK_SHADER_STAGE_VERTEX_BIT
                    );
                    ///use the shadow map samplers
                    std::vector<VkDescriptorSet> samplerDescriptorSetRingBuffer = descriptorService.DescriptorSet(vk::FAKE_SHADOW_MAP_SAMPLERS);
                    vkCmdBindDescriptorSets(
                        commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                        pipeline.PipelineLayout(),
                        vk::FAKE_SHADOW_MAP_OUTPUT_SAMPLERS_SET,
                        1,
                        &samplerDescriptorSetRingBuffer[currentFrame],
                        0,
                        nullptr
                    );
                }
            };
            gObjects.push_back(tile);
            tiles.push_back(tile);
        }
    }
    auto gBox = new entities::GameObject("box", descriptorService, meshService.GetMesh("box.glb"));
    gBox->SetPosition(glm::vec3(3, 3, 0));
    gBox->SetOrientation(glm::quat());
    gObjects.push_back(gBar);
    gBox->OnDraw = [&descriptorService](entities::GameObject& go, entities::Pipeline& pipeline, uint32_t currentFrame, VkCommandBuffer commandBuffer) {
        if (pipeline.Hash() == utils::Hash("phongSolidColor")) {
            entities::ColorPushConstantData color{ {0,1,1,1} };
            pipeline.SetPushConstant<entities::ColorPushConstantData>(
                color, go.mId, commandBuffer, VK_SHADER_STAGE_VERTEX_BIT
            );
            ///use the shadow map samplers
            std::vector<VkDescriptorSet> samplerDescriptorSetRingBuffer = descriptorService.DescriptorSet(vk::FAKE_SHADOW_MAP_SAMPLERS);
            vkCmdBindDescriptorSets(
                commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                pipeline.PipelineLayout(),
                vk::FAKE_SHADOW_MAP_OUTPUT_SAMPLERS_SET,
                1,
                &samplerDescriptorSetRingBuffer[currentFrame],
                0,
                nullptr
            );
        }
    };

    std::vector<entities::GameObject*> objectsWithShadowAndLight(tiles);
    objectsWithShadowAndLight.push_back(gFoo);
    objectsWithShadowAndLight.push_back(gBox);
    //add the game objects to their pipelines
    gPipelineGameObjectTable.insert({ phongSolidColor->Hash(), objectsWithShadowAndLight});
    gPipelineGameObjectTable.insert({ demoPipeline->Hash(), {gBar} });
    gPipelineGameObjectTable.insert({ fakeShadowMapPipeline->Hash(), objectsWithShadowAndLight });
    //Define the camera
    
    cameraBuffer.cameraPos = glm::vec3(-7.0f, -5.0f, 7.0f);
    cameraBuffer.view = glm::lookAt(cameraBuffer.cameraPos, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    //some perspective projection
    cameraBuffer.proj = glm::perspective(glm::radians(45.0f),
        swapChain.GetExtent().width / (float)swapChain.GetExtent().height, 0.01f, 500.0f);
    //GOTCHA: GLM is for opengl, the y coords are inverted. With this trick we the correct that
    cameraBuffer.proj[1][1] *= -1;
    //define the main loop callback
    app::Timer timer;
    size_t currentFrame = 0;
    uint32_t imageIndex = UINT32_MAX;
    std::vector<VkCommandBuffer> commandBuffers = device.CreateCommandBuffer("mainCommandBuffers", MAX_FRAMES_IN_FLIGHT);


    mainWindow.OnRender = [&timer, &syncService, &currentFrame, 
        &swapChain, &mainFramebuffer,&fakeShadowMapFramebuffer, &descriptorService]
        (app::Window* w) 
        {
            timer.Advance();//advance the clock
            entities::Frame frame(currentFrame, syncService, swapChain);
            frame.BeginFrame();
            for (auto R : gOrderedRenderpasses) {
                //for each render pass R begin R, but remember that there are many different types of framebuffers
                //and we must use the one compatible with the render pass.
                //TODO optization: string comparison is bad. Use some kind of id or hash
                if (mainFramebuffer.mRenderPass.mName == R->mName) {
                    R->BeginRenderPass({ 0,0,0,1 }, { 1.0f, 0 },
                        mainFramebuffer.GetFramebuffer(frame.ImageIndex()), { SCREEN_WIDTH,SCREEN_HEIGH },
                        frame.CommandBuffer(), currentFrame);
                }
                if (fakeShadowMapFramebuffer.mRenderPass.mName == R->mName) {
                    R->BeginRenderPass({ 0,0,0,1 }, { 1.0f, 0 },
                        fakeShadowMapFramebuffer.GetFramebuffer(frame.ImageIndex()%MAX_FRAMES_IN_FLIGHT), { SHADOW_MAP_WIDTH, SHADOW_MAP_HEIGHT},
                        frame.CommandBuffer(), currentFrame);
                }
                //for each pipeline P of R bind P
                std::vector<pipeline_hash_t> pipelineHashes = gRenderPassPipelineTable.at(utils::Hash(R->mName));
                std::vector<entities::Pipeline*> pipelines(pipelineHashes.size());
                std::transform(pipelineHashes.begin(), pipelineHashes.end(), pipelines.begin(), [](auto h) {
                    return gPipelines.at(h);
                });
                for (auto P : pipelines) {
                    // Bind the pipeline
                    P->Bind(frame.CommandBuffer(), frame.mCurrentFrame);
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
                R->EndRenderPass(frame.CommandBuffer(), currentFrame);
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
    entities::Pipeline* demoPipeline = (new entities::PipelineBuilder("demoPipeline", descriptorService))->
        SetRenderPass(renderPass)->
        SetShaderModules(
            entities::LoadShaderModule(device.GetDevice(), "demo.vert.spv"),
            entities::LoadShaderModule(device.GetDevice(), "demo.frag.spv")
        )->
        SetCameraCallback([](uintptr_t destAddr) {
            void* cameraDescriptorSetAddr = reinterpret_cast<void*>(destAddr);
            memcpy(cameraDescriptorSetAddr, &cameraBuffer, sizeof(entities::CameraUniformBuffer));
        })->
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
    entities::Pipeline* phongSolidColor = (new entities::PipelineBuilder("phongSolidColor", descriptorService))->
        SetPushConstantRanges({ entities::GetPushConstantRangeFor<entities::ColorPushConstantData>(VK_SHADER_STAGE_VERTEX_BIT) })->
        SetRenderPass(renderPass)->
        SetShaderModules(
            entities::LoadShaderModule(device.GetDevice(), "phong_color.vert.spv"),
            entities::LoadShaderModule(device.GetDevice(), "phong_color.frag.spv")
        )->
        SetCameraCallback([](uintptr_t destAddr) {
        void* cameraDescriptorSetAddr = reinterpret_cast<void*>(destAddr);
        memcpy(cameraDescriptorSetAddr, &cameraBuffer, sizeof(entities::CameraUniformBuffer));
            })->
        SetDescriptorSetLayouts({ 
            descriptorService.DescriptorSetLayout(vk::CAMERA_LAYOUT_NAME),//We need the camera
            descriptorService.DescriptorSetLayout(vk::MODEL_MATRIX_LAYOUT_NAME), //and the model matrix
            descriptorService.DescriptorSetLayout(vk::LIGHTNING_LAYOUT_NAME), //and lightning data
            descriptorService.DescriptorSetLayout(vk::FAKE_SHADOW_MAP_SAMPLERS) //and the samplers for the 
            })->
            SetVertexInputStateInfo(entities::GetVertexInputInfoForMesh())->
        SetRasterizerStateInfo(entities::GetBackfaceCullClockwiseRasterizationInfo())->
        SetDepthStencilStateInfo(entities::GetDefaultDepthStencil())->
        SetColorBlending(entities::GetNoColorBlend())->
        SetViewport(entities::GetViewportForSize(SCREEN_WIDTH, SCREEN_HEIGH), entities::GetScissor(SCREEN_WIDTH, SCREEN_HEIGH))-> //TODO resize: hardcoded screen size
        SetLightDataCallback(lightCallback)->
        Build();
    return phongSolidColor;
}
entities::Pipeline* CreateShadowMapPipeline(vk::RenderPass* renderPass, vk::Device& device, vk::DescriptorService& descriptorService, uint32_t w, uint32_t h)
{
    entities::Pipeline* p = (new entities::PipelineBuilder("fakeShadowMapPipeline", descriptorService))->
        SetRenderPass(renderPass)->
        SetShaderModules(
            entities::LoadShaderModule(device.GetDevice(), "shadow_map.vert.spv"),
            entities::LoadShaderModule(device.GetDevice(), "shadow_map.frag.spv"))
        ->SetDescriptorSetLayouts({
            descriptorService.DescriptorSetLayout(vk::CAMERA_LAYOUT_NAME),
            descriptorService.DescriptorSetLayout(vk::MODEL_MATRIX_LAYOUT_NAME)})->
        SetVertexInputStateInfo(entities::GetVertexInputInfoForMesh())->
        SetRasterizerStateInfo(entities::GetBackfaceCullClockwiseRasterizationInfo())->
        SetDepthStencilStateInfo(entities::GetDefaultDepthStencil())->
        SetColorBlending(entities::GetNoColorBlend())->
        SetViewport(entities::GetViewportForSize(w, h), entities::GetScissor(w, h))->
        SetCameraCallback([](uintptr_t destAddr) {
            ////light matrix calc
            glm::mat4 projection = glm::ortho(
                -orthoSize, orthoSize, -orthoSize, orthoSize,1.0f, 200.f);
            glm::vec3 lightDirection = glm::normalize(lights.directionalLights.direction);
            glm::vec3 originInInf = glm::vec3(0, 0, 100.0f);
            glm::mat4 viewMatrix = glm::lookAt(originInInf, 
                originInInf + lightDirection, 
                glm::vec3(0,0,1));
            //GOTCHA: GLM is for opengl, the y coords are inverted. With this trick we the correct that
            projection[1][1] *= -1;
            entities::CameraUniformBuffer lightAsCamera;
            lightAsCamera.proj = projection;
            lightAsCamera.view = viewMatrix;
            lightAsCamera.cameraPos = originInInf;
            void* cameraDescriptorSetAddr = reinterpret_cast<void*>(destAddr);
            memcpy(cameraDescriptorSetAddr, &lightAsCamera, sizeof(entities::CameraUniformBuffer));
        })->
        Build();
    return p;

    
}
