#pragma once
#include <vulkan/vulkan.h>
namespace utils {
    uint32_t FindMemoryType(uint32_t typeFilter,
        VkMemoryPropertyFlags properties);

    VkFormat FindDepthFormat(VkPhysicalDevice physicalDevice);

    void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
        VkMemoryPropertyFlags properties, VkBuffer& buffer,
        VkDeviceMemory& bufferMemory);
}