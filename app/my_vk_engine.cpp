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
std::vector<entities::GameObject*> gObjects{};
std::map<size_t, entities::Pipeline*> gPipelines;
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
    vk::RenderPass mainRenderPass( swapChain.GetFormat(), "mainRenderPass");
    //create the framebuffer for onscreen rendering
    //TODO depth buffer - no depth buffer, need to add one
    vk::Framebuffer mainFramebuffer(swapChain.GetImageViews(), swapChain.GetExtent(), mainRenderPass, "mainFramebuffer");
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
    entities::Pipeline* demoPipeline = (new entities::PipelineBuilder("demoPipeline"))->
        SetRenderPass(&mainRenderPass)->
        SetShaderModules(
            entities::LoadShaderModule(device.GetDevice(), "demo.vert.spv"),
            entities::LoadShaderModule(device.GetDevice(), "demo.frag.spv")
        )->
        SetDescriptorSetLayouts({
            descriptorService.DescriptorSetLayout(vk::CAMERA_LAYOUT_NAME),
            descriptorService.DescriptorSetLayout(vk::OBJECT_LAYOUT_NAME)})->
        SetVertexInputStateInfo(entities::GetVertexInputInfoForMesh())->
        SetRasterizerStateInfo(entities::GetBackfaceCullClockwiseRasterizationInfo())->
        SetDepthStencilStateInfo(entities::GetDefaultDepthStencil())->
        SetColorBlending(entities::GetNoColorBlend())->
        SetViewport(entities::GetViewportForSize(SCREEN_WIDTH,SCREEN_HEIGH), entities::GetScissor(SCREEN_WIDTH,SCREEN_HEIGH))-> //TODO resize: hardcoded screen size
        Build();
    gPipelines.insert({ utils::Hash("demoPipeline"), demoPipeline });
    //create the synchronization objects
    vk::SyncronizationService syncService;
    //Create a game object
    auto gFoo = new entities::GameObject("foo", descriptorService,"demoPipeline", meshService.GetMesh("monkey.glb"));
    gFoo->SetPosition(glm::vec3( 0,0,0 ));
    gFoo->SetOrientation(glm::quat());
    gObjects.push_back(gFoo);
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
        &swapChain, &mainRenderPass, &mainFramebuffer, &descriptorService, &cameraBuffer]
        (app::Window* w) 
        {
            timer.Advance();//advance the clock
            entities::Frame frame(currentFrame, syncService, swapChain);
            frame.BeginFrame();
            mainRenderPass.BeginRenderPass({ 1,0,1,1 }, { 1.0f, 0 }, 
                mainFramebuffer.GetFramebuffer(frame.ImageIndex()), { SCREEN_WIDTH,SCREEN_HEIGH }, 
            frame.CommandBuffer());
            //set the camera data for the main render pass, all objects will use the same camera
            std::vector<uintptr_t> cameraDescriptorSetsAddrs = descriptorService.DescriptorSetsBuffersAddrs(vk::CAMERA_LAYOUT_NAME, 0);
            void* cameraDescriptorSetAddr = reinterpret_cast<void*>(cameraDescriptorSetsAddrs[currentFrame]);
            memcpy(cameraDescriptorSetAddr, &cameraBuffer, sizeof(entities::CameraUniformBuffer));
            //bind the pipeline
            auto pipeline = gPipelines.at(utils::Hash("demoPipeline"));
            pipeline->Bind(frame.CommandBuffer());
            //draw the objects
            for (auto& go : gObjects) {
                go->Draw(frame.CommandBuffer(), *pipeline , currentFrame);
            }
            mainRenderPass.EndRenderPass(frame.CommandBuffer());
            frame.EndFrame();
            currentFrame = currentFrame + 1;
            currentFrame = currentFrame % MAX_FRAMES_IN_FLIGHT;
    };
    ///begin the main loop - blocks here
    mainWindow.MainLoop();
    return 0;
}