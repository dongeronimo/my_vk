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
typedef hash_t renderpass_hash_t;
typedef hash_t pipeline_hash_t;
const uint32_t SHADOW_MAP_WIDTH = 512;
const uint32_t SHADOW_MAP_HEIGHT = 512;
std::vector<vk::DepthBuffer*> gDepthBuffersForShadowMapping(MAX_FRAMES_IN_FLIGHT);
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
    //create the render pass for onscreen rendering
    vk::RenderPass* mainRenderPass = new vk::RenderPass(utils::FindDepthFormat(instance.GetPhysicalDevice()), swapChain.GetFormat(),"mainRenderPass");
    mainRenderPass->mOnRenderPassEndCallback = [](vk::RenderPass* renderPass, VkCommandBuffer cmdBuffer, uint32_t currentFrame) {
        //When we end the main render pass we must revert the images used by the shadow map pass back to the layout that it expects them to be.
        VkImageMemoryBarrier barrier = {};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        barrier.image = gDepthBuffersForShadowMapping[currentFrame]->GetImage();
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        vkCmdPipelineBarrier(
            cmdBuffer,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,      // Source stage: after shader sampling
            VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, // Destination stage: before depth attachment write
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier
        );
    };
    //Create the render pass for shadow mapping
    vk::RenderPass* shadowMappingRenderPass = vk::RenderPass::RenderPassForShadowMapping("shadowMapRenderPass");
    shadowMappingRenderPass->mOnRenderPassEndCallback = [](vk::RenderPass* renderPass, VkCommandBuffer cmdBuffer, uint32_t currentFrame) {
        //The shadow map needs to transition the depth buffer to an appropriate layout once it's finished
         VkImageMemoryBarrier barrier = {};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;//it's in depth stencil layout
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; //must go to the layout that shaders understand
        barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        barrier.image = gDepthBuffersForShadowMapping[currentFrame]->GetImage();
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        vkCmdPipelineBarrier(
            cmdBuffer,
            VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,  // Source stage: depth write
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,      // Destination stage: shadow map sampling
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier
        );
    };
    gOrderedRenderpasses.push_back(shadowMappingRenderPass);
    gOrderedRenderpasses.push_back(mainRenderPass);
    //create the framebuffer for onscreen rendering
    ///Creates the depth buffers for the main framebuffer, one per frame
    vk::DepthBuffer depthBuffer(swapChain.GetExtent().width, swapChain.GetExtent().height);
    VkImageView depthBufferImageView = depthBuffer.GetImageView();
    vk::Framebuffer mainFramebuffer(swapChain.GetImageViews(), depthBufferImageView,
        swapChain.GetExtent(), *mainRenderPass, "mainFramebuffer");
    /////////Create the framebuffer for shadow mapping
    //First, create the buffers
    
    std::vector<VkImageView> imageViewsForShadowMapping(MAX_FRAMES_IN_FLIGHT);
    for (auto i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        gDepthBuffersForShadowMapping[i] = new vk::DepthBuffer(SHADOW_MAP_WIDTH,
            SHADOW_MAP_HEIGHT, VK_FORMAT_D32_SFLOAT);
        imageViewsForShadowMapping[i] = gDepthBuffersForShadowMapping[i]->GetImageView();
    }
    //Then create the framebuffer
    vk::Framebuffer shadowMapFramebuffer(imageViewsForShadowMapping, 
        { SHADOW_MAP_WIDTH, SHADOW_MAP_HEIGHT }, 
        *shadowMappingRenderPass, "shadowMapFrameBuffer");

    ////////////////////
    //create the samplers
    vk::SamplerService samplersService;
    //create the images
    vk::ImageService imageService({"blackBrick.png","brick.png","floor01.jpg"});
    //create the descriptor infrastructure: descriptorSetLayouts, DescriptorPools and DescriptorSets
    vk::DescriptorService descriptorService(samplersService,
        imageService);
    //load meshes
    vk::MeshService meshService({ "monkey.glb", "4x4tile.glb"});
    //create the pipelines
    entities::Pipeline* demoPipeline = CreateDemoPipeline(mainRenderPass, device, descriptorService);
    entities::TLightCallback lightCallback = []() {
        lights.ambient.colorAndIntensity = { 1,1,1,0.1f };
        lights.directionalLights.diffuseColorAndIntensity[0] = { 1,1,1,1 };
        lights.directionalLights.direction[0] = { 0,0,-1 };
        return lights;
    };
    entities::Pipeline* phongSolidColor = CreatePhongSolidColorPipeline(mainRenderPass, device, descriptorService, lightCallback);  
    entities::Pipeline* shadowMapPipeline = CreateShadowMapPipeline(
        shadowMappingRenderPass, device, descriptorService, SHADOW_MAP_WIDTH, SHADOW_MAP_HEIGHT);
    gPipelines.insert({ utils::Hash("phongSolidColor") , phongSolidColor });
    gPipelines.insert({ utils::Hash("demoPipeline"), demoPipeline });
    gPipelines.insert({ utils::Hash("shadowMapPipeline"), shadowMapPipeline });
    //register that mainRenderPass has these pipelines
    gRenderPassPipelineTable.insert({ utils::Hash("mainRenderPass"), {utils::Hash("phongSolidColor"),  utils::Hash("demoPipeline")} });
    gRenderPassPipelineTable.insert({ utils::Hash("shadowMapRenderPass"), {utils::Hash("shadowMapPipeline")} });
    //create the synchronization objects
    vk::SyncronizationService syncService;
    //////////////////////GAME OBJECTS//////////////////////
    //Create a game object
    auto gFoo = new entities::GameObject("foo", descriptorService, meshService.GetMesh("monkey.glb"));
    gFoo->SetPosition(glm::vec3( 0,4,0 ));
    gFoo->SetOrientation(glm::quat());
    gFoo->OnDraw = [](entities::GameObject& go, entities::Pipeline& pipeline, uint32_t currentFrame, VkCommandBuffer commandBuffer) {
        if (pipeline.Hash() == utils::Hash("phongSolidColor")) { //a very rudimentary material system, passing the color that i want.
            entities::ColorPushConstantData color{ {0,1,0,1} };
            pipeline.SetPushConstant<entities::ColorPushConstantData>(
                color, go.mId, commandBuffer, VK_SHADER_STAGE_VERTEX_BIT
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
            tile->SetPosition(glm::vec3{ i * 2, j * 2, 0.0f }+ glm::vec3{-2.5f, -2.5f, -4.0f});
            tile->SetOrientation(glm::quat());
            tile->OnDraw = [i, j](entities::GameObject& go, entities::Pipeline& pipeline, uint32_t currentFrame, VkCommandBuffer commandBuffer) {
                if (pipeline.Hash() == utils::Hash("phongSolidColor")) {
                    glm::vec4 _color = { (float)i / 5.0f, (float)j / 5.0f, 0.24f, 1 };
                    entities::ColorPushConstantData color{ _color };
                    pipeline.SetPushConstant<entities::ColorPushConstantData>(
                        color, go.mId, commandBuffer, VK_SHADER_STAGE_VERTEX_BIT
                    );
                }
            };
            gObjects.push_back(tile);
            tiles.push_back(tile);
        }
    }
    std::vector<entities::GameObject*> objectsWithShadowAndLight(tiles);
    objectsWithShadowAndLight.push_back(gFoo);
    //add the game objects to their pipelines
    gPipelineGameObjectTable.insert({ phongSolidColor->Hash(), objectsWithShadowAndLight});
    gPipelineGameObjectTable.insert({ demoPipeline->Hash(), {gBar} });
    gPipelineGameObjectTable.insert({ shadowMapPipeline->Hash(), objectsWithShadowAndLight });
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
        &swapChain, &mainFramebuffer,&shadowMapFramebuffer, &descriptorService]
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
                if (shadowMapFramebuffer.mRenderPass.mName == R->mName) {
                    R->BeginRenderPass({ 0,0,0,1 }, { 1.0f, 0 },
                        shadowMapFramebuffer.GetFramebuffer(frame.ImageIndex()%MAX_FRAMES_IN_FLIGHT), { SHADOW_MAP_WIDTH, SHADOW_MAP_HEIGHT},
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

entities::Pipeline* CreateShadowMapPipeline(vk::RenderPass* renderPass, vk::Device& device, vk::DescriptorService& descriptorService, uint32_t w, uint32_t h)
{
    entities::Pipeline* p = (new entities::PipelineBuilder("shadowMapPipeline", descriptorService))->
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
            glm::mat4 orthoMatrix = glm::ortho(-SHADOW_MAP_WIDTH/2.0f, SHADOW_MAP_WIDTH/2.0f, SHADOW_MAP_HEIGHT/2.0f, -SHADOW_MAP_HEIGHT/2.0f, 0.01f, 200.f);
            glm::vec3 lightDirection = glm::normalize(lights.directionalLights.direction[0]);
            glm::vec3 originInInf = lightDirection * -100.f;
            glm::mat4 viewMatrix = glm::lookAt(originInInf, glm::vec3(0,0,0), glm::vec3(0,0,1));

            entities::CameraUniformBuffer lightAsCamera;
            lightAsCamera.proj = orthoMatrix;
            lightAsCamera.view = viewMatrix;
            lightAsCamera.cameraPos = originInInf;
            void* cameraDescriptorSetAddr = reinterpret_cast<void*>(destAddr);
            memcpy(cameraDescriptorSetAddr, &lightAsCamera, sizeof(entities::CameraUniformBuffer));
        })->
        Build();
    return p;
}
