#pragma once
#include <vulkan/vulkan.h>
namespace vk {
    class DepthBuffer {
    public:
        DepthBuffer(uint32_t w, uint32_t h, VkFormat format);
        DepthBuffer(uint32_t w, uint32_t h);
        ~DepthBuffer();
        VkImage GetImage()const;
        VkImageView GetImageView()const;
        VkFormat GetFormat()const;
    private:
        VkFormat mDepthFormat;
        VkImage mImage = VK_NULL_HANDLE;
        VkImageView mImageView = VK_NULL_HANDLE;
        VkDeviceMemory mDeviceMemory = VK_NULL_HANDLE;
    };
}