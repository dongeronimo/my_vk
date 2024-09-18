#include "vk_utils.h"
#include <vk/instance.h>

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
