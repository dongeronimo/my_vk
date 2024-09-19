#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <string>
#include <entities/image.h>
#include <map>
namespace vk {
    class ImageService {
    public:
        static VkImage CreateImage(VkDevice device, uint32_t w, uint32_t h, VkFormat format,
            VkImageUsageFlags usage);
        static uint32_t FindMemoryTypesCompatibleWithAllImages(
            const std::vector<VkMemoryRequirements>& memoryRequirements);
        static void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
            VkMemoryPropertyFlags properties, VkBuffer& buffer,
            VkDeviceMemory& bufferMemory, VkDevice device);
        static void TransitionImageLayout(VkImage image, VkFormat format,
            VkImageLayout oldLayout, VkImageLayout newLayout,
            VkCommandPool commandPool, VkDevice device, VkQueue graphicsQueue);
        static void CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
        ImageService(const std::vector<std::string>& assetNames);

    private:
        VkDeviceMemory mDeviceMemory = VK_NULL_HANDLE;
        std::map<size_t, entities::Image> mImageTable;
    };
}