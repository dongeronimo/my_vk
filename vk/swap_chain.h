#pragma once
#include <vulkan/vulkan.h>
#include <vector>
namespace vk 
{
    /// <summary>
    /// To render to screen we need the swap chain. The swap chain is
    /// the set of buffer images, like front buffer and back buffer where
    /// draw operation results are stored.
    /// </summary>
    class SwapChain {
    public:
        /// <summary>
        /// It assumes that the instance and the device have been initialized
        /// </summary>
        SwapChain();
        ~SwapChain();
        VkExtent2D GetExtent()const { return mSwapChainExtent; }
        VkSwapchainKHR& GetSwapChain() { return mSwapChain; }
        VkImage GetImage(uint32_t which)const { return mSwapChainImages[which]; }
        VkImageView GetImageView(uint32_t which)const { return mSwapChainImageViews[which]; }
        std::vector<VkImageView> GetImageViews()const { return mSwapChainImageViews; }
        VkFormat GetFormat()const { return mImageFormat; }
    private:
        VkExtent2D mSwapChainExtent;
        VkSwapchainKHR mSwapChain = VK_NULL_HANDLE;
        std::vector<VkImage> mSwapChainImages;
        std::vector<VkImageView> mSwapChainImageViews;
        VkFormat mImageFormat;
    };
}