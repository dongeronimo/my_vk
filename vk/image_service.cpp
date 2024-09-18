#include "image_service.h"
#include "io/image-load.h"
#include <algorithm>
#include "device.h"
#include "instance.h"
namespace vk
{
    ImageService::ImageService(const std::vector<std::string>& assetNames)
    {
        //load the images from the file to the memory
        std::vector<io::ImageData*> imageDataFromFiles(assetNames.size());
        std::transform(assetNames.begin(), assetNames.end(), imageDataFromFiles.begin(),
            [](const std::string& name) {
                io::ImageData* img = io::LoadImage(name);
                return img;
            });
        //grab vars that'll need to push the images from the memory to the gpu
        VkDevice device = vk::Device::gDevice->GetDevice();
        VkPhysicalDevice physicalDevice = vk::Instance::gInstance->GetPhysicalDevice();
        VkCommandPool commandPool = vk::Device::gDevice->GetCommandPool();
        VkQueue graphicsQueue = vk::Device::gDevice->GetGraphicsQueue();
    }
}