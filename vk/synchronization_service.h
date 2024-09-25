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
        VkSemaphore& ImagesAvailableSemaphore(uint32_t i);
        VkSemaphore& RenderFinishedSemaphore(uint32_t i);
        VkFence& InFlightFences(uint32_t i);
    private:
        std::vector<VkSemaphore> mImagesAvailableSemaphore;
        std::vector<VkSemaphore> mRenderFinishedSemaphore;
        std::vector<VkFence> mInFlightFence;
    };
}