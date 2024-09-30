#include "depth_buffer.h"
#include "image_service.h"
#include "device.h"
#include <vector>
#include <utils\vk_utils.h>
#include "instance.h"
namespace vk
{

    DepthBuffer::DepthBuffer(uint32_t w, uint32_t h)
    {
        auto device = Device::gDevice->GetDevice();
        auto physicalDevice = Instance::gInstance->GetPhysicalDevice();
        mDepthFormat = utils::FindDepthFormat(physicalDevice); // Choose an appropriate depth format
        ImageService::CreateImage(w, h, mDepthFormat,
            VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, mImage, mDeviceMemory);
        mImageView = ImageService::CreateImageView(mImage, mDepthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
        utils::TransitionImageLayout(mImage, mDepthFormat,
            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
        );
    }

    DepthBuffer::~DepthBuffer()
    {
        auto device = Device::gDevice->GetDevice();
        vkDestroyImage(device, mImage, nullptr);
        vkDestroyImageView(device, mImageView, nullptr);
        vkFreeMemory(device, mDeviceMemory, nullptr);
    }

    VkImageView DepthBuffer::GetImageView() const
    {
        return mImageView;
    }

    VkFormat DepthBuffer::GetFormat() const
    {
        return mDepthFormat;
    }
    
}