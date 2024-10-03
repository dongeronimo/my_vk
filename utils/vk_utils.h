#pragma once
#include <vulkan/vulkan.h>
namespace utils {
    uint32_t FindMemoryType(uint32_t typeFilter,
        VkMemoryPropertyFlags properties);

    VkFormat FindDepthFormat(VkPhysicalDevice physicalDevice);

    void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
        VkMemoryPropertyFlags properties, VkBuffer& buffer,
        VkDeviceMemory& bufferMemory);

    void TransitionImageLayout(VkImage image, VkFormat format,
        VkImageLayout oldLayout, VkImageLayout newLayout);

    void TransitionImageLayout(VkCommandBuffer commandBuffer, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout,
        VkImageAspectFlags subresourceAspectMask, VkAccessFlags srcAccessMask, VkAccessFlags destAccessMask, VkPipelineStageFlags sourceStage, VkPipelineStageFlags destinationStage);

}