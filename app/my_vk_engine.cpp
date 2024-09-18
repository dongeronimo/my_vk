#include "app/window.h"
#include "vk/instance.h"
#include "vk/device.h"
#include "vk/debug_utils.h"
#include "vk/swap_chain.h"
#include "vk/descriptor_service.h"
#include "vk/samplers_service.h"
#include "vk/image_service.h"
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
    //create the samplers
    vk::SamplerService samplersService;
    //create the images
    vk::ImageService imageService({"blackBrick.png","brick.png","floor01.jpg"});
    //create the descriptor infrastructure: descriptorSetLayouts, DescriptorPools and DescriptorSets
    vk::DescriptorService descriptorService(samplersService,
        imageService);
    //begin the main loop - blocks here
    mainWindow.MainLoop();
    return 0;
}