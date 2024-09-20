#include "vk_utils.h"
#include <vk/instance.h>
#include <vk/device.h>

uint32_t utils::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
    assert(vk::Instance::gInstance != nullptr);
    assert(vk::Instance::gInstance->GetPhysicalDevice() != VK_NULL_HANDLE);
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(vk::Instance::gInstance->GetPhysicalDevice(), &memProperties);
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    throw std::runtime_error("failed to find suitable memory type!");
}
VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features, VkPhysicalDevice physicalDevice)
{
    for (VkFormat format : candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);
        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
            return format;
        }
        else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }
    throw std::runtime_error("failed to find supported format!");
}
VkFormat utils::FindDepthFormat(VkPhysicalDevice physicalDevice)
{
    return findSupportedFormat(
        { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT,
        physicalDevice
    );
}

void utils::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory)
{
    assert(size != 0);//size must not be zero
    //Buffer description
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    //create the buffer
    if (vkCreateBuffer(vk::Device::gDevice->GetDevice(), &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to create buffer!");
    }
    //the memory the buffer will require, not necessarely equals to the size of the data being stored
    //in it.
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(vk::Device::gDevice->GetDevice(), buffer, &memRequirements);
    //TODO: Use VulkanMemoryAllocator instead of allocating memory on my own because gpus have a limited number of simultaneous memory allocations.
    //In a scene with a lot of objects this limit is reached very fast
    //https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties);
    //Allocate the memory
    if (vkAllocateMemory(vk::Device::gDevice->GetDevice(), &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate buffer memory!");
    }

    vkBindBufferMemory(vk::Device::gDevice->GetDevice(), buffer, bufferMemory, 0);
}
