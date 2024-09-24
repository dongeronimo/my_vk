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
std::vector<entities::GameObect*> gObjects{};
std::map<size_t, entities::Pipeline*> gPipelines;
int main(int argc, char** argv)
{
    glfwInit();
    app::Window mainWindow(1024, 762);
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
        SetViewport(entities::GetViewportForSize(1024,768), entities::GetScissor(1024,768))-> //TODO resize: hardcoded screen size
        Build();
    gPipelines.insert({ utils::Hash("demoPipeline"), demoPipeline });
    //create the synchronization objects
    vk::SyncronizationService syncService;
    //Create a game object
    auto gFoo = new entities::GameObect("foo", descriptorService,"demoPipeline", meshService.GetMesh("monkey.glb"));
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
    mainWindow.OnRender = [&timer, &syncService, &currentFrame, 
        &swapChain, &mainRenderPass, &mainFramebuffer, &descriptorService, &cameraBuffer]
        (app::Window* w) {
        timer.Advance();//advance the clock
        entities::Frame frame(currentFrame, syncService, swapChain);
        if (frame.BeginFrame()) 
        {
            vk::SetMark({ 0.2f, 0.8f, 0.1f }, "OnScreenRenderPass", frame.CommandBuffer());
            mainRenderPass.BeginRenderPass({ 1,0,1,1 }, { 1.0f, 0 }, 
                mainFramebuffer.GetFramebuffer(currentFrame), { 1024,752 }, frame.CommandBuffer());//TODO resize: hardcoded screen size
            //render things
            entities::Pipeline* pipeline = gPipelines.at(utils::Hash("demoPipeline"));
            //set the camera data that i'll use
            std::vector<uintptr_t> cameraUniformBuffersAddr = descriptorService.DescriptorSetsBuffersAddrs(vk::CAMERA_LAYOUT_NAME, 0);
            void* cameraDstAddr = reinterpret_cast<void*>(cameraUniformBuffersAddr[currentFrame]);
            memcpy(cameraDstAddr, &cameraBuffer, sizeof(entities::CameraUniformBuffer));

            auto go = gObjects[0];
            pipeline->Bind(frame.CommandBuffer());
            go->Draw(frame.CommandBuffer());
            
            //end the render pass
            vk::EndMark(frame.CommandBuffer());
            mainRenderPass.EndRenderPass(frame.CommandBuffer());
            //end the frame
            frame.EndFrame();
        }
        currentFrame = currentFrame + 1;
        currentFrame = currentFrame % MAX_FRAMES_IN_FLIGHT;
    };
    //begin the main loop - blocks here
    mainWindow.MainLoop();
    return 0;
}