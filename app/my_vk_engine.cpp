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
    //TODO:create the pipelines
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
        Build();
    //create the synchronization objects
    vk::SyncronizationService syncService;
    //TODO: Create a game object
    //TODO: Define the camera
    //begin the main loop - blocks here
    mainWindow.MainLoop();
    return 0;
}