#pragma once
#include <vulkan/vulkan.h>
namespace vk {
    class SyncronizationService;
    class SwapChain;
}
namespace entities {
    class Frame
    {
    public:
        Frame(
            size_t currentFrame,
            vk::SyncronizationService& syncService,
            vk::SwapChain& swapChain);
        bool BeginFrame();
        const size_t mCurrentFrame;
        VkCommandBuffer CommandBuffer();
        void EndFrame();
    private:
        vk::SyncronizationService& mSyncService;
        vk::SwapChain& mSwapChain;
        bool IsDegenerateFramebuffer() const;
        uint32_t mImageIndex;
    };
}