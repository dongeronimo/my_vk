#pragma once
#include <vector>
#include <vulkan/vulkan.h>
namespace vk
{
    class SyncronizationService
    {
    public:
        SyncronizationService();
        ~SyncronizationService();
        VkSemaphore ImagesAvailableSemaphore(uint32_t i)const;
        VkSemaphore RenderFinishedSemaphore(uint32_t i)const;
        VkFence InFlightFences(uint32_t i)const;
    private:
        std::vector<VkSemaphore> mImagesAvailableSemaphore;
        std::vector<VkSemaphore> mRenderFinishedSemaphore;
        std::vector<VkFence> mInFlightFence;
    };
}