#include "app/window.h"
#include "vk/instance.h"
#include "vk/device.h"
#include "vk/debug_utils.h"
#include "vk/swap_chain.h"
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
    //begin the main loop - blocks here
    mainWindow.MainLoop();
    return 0;
}