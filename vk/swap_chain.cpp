#include "vk\swap_chain.h"
#include "vk\instance.h"
#include "vk\device.h"
#include "vk\debug_utils.h"
#include "utils\concatenate.h"
#include <cassert>

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};
SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface)
{
    SwapChainSupportDetails details;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);
    //get the number of surface formats
    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
    //get the surface formats
    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
    }
    //get the presentation modes
    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount,
            details.presentModes.data());
    }
    return details;
}

VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
            availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            return availableFormat;
        }
    }
    //TODO swapchain: Support other image formats, like linear rgb
    throw new std::runtime_error("I only accept VK_FORMAT_B8G8R8A8_SRGB non linear for the swap chain framebuffers");
}

VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
{
    //TODO swapchain: prefer MAILBOX but if no MAILBOX use FIFO
    return VK_PRESENT_MODE_FIFO_KHR;
}
namespace vk {
    SwapChain::SwapChain()
    {
        assert(vk::Instance::gInstance != nullptr);
        assert(vk::Device::gDevice != nullptr);
        VkPhysicalDevice physicalDevice = vk::Instance::gInstance->GetPhysicalDevice();
        const vk::Device* _device = vk::Device::gDevice;
        VkSurfaceKHR surface = vk::Instance::gInstance->GetSurface();
        SwapChainSupportDetails supportDetails = QuerySwapChainSupport(physicalDevice, surface);
        VkSurfaceFormatKHR chosenSurfaceFormat = ChooseSwapSurfaceFormat(supportDetails.formats);
        VkPresentModeKHR chosenPresentMode = ChooseSwapPresentMode(supportDetails.presentModes);
        if (supportDetails.capabilities.currentExtent.width != UINT32_MAX) {
            mSwapChainExtent = supportDetails.capabilities.currentExtent;
        }
        else {
            mSwapChainExtent = { 1024, 768 };
            mSwapChainExtent.width = std::max(supportDetails.capabilities.minImageExtent.width,
                std::min(supportDetails.capabilities.maxImageExtent.width, mSwapChainExtent.width));
            mSwapChainExtent.height = std::max(supportDetails.capabilities.minImageExtent.height,
                std::min(supportDetails.capabilities.maxImageExtent.height, mSwapChainExtent.height));
        }
        //sets the number of swap chain images to be either min+1 or the max number possible
        uint32_t imageCount = supportDetails.capabilities.minImageCount + 1;
        if (supportDetails.capabilities.maxImageCount > 0 &&
            imageCount > supportDetails.capabilities.maxImageCount) {
            imageCount = supportDetails.capabilities.maxImageCount;
        }
        //the swap chain properties
        VkSwapchainCreateInfoKHR swapchainCreateInfo{};
        swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        swapchainCreateInfo.surface = surface;
        swapchainCreateInfo.minImageCount = imageCount;
        swapchainCreateInfo.imageFormat = chosenSurfaceFormat.format;
        swapchainCreateInfo.imageColorSpace = chosenSurfaceFormat.colorSpace;
        swapchainCreateInfo.imageExtent = mSwapChainExtent;
        swapchainCreateInfo.imageArrayLayers = 1;
        swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        //this changes whether the queue families are different or not
        auto graphicsQueueFamily = _device->GetGraphicsQueueFamily();
        auto presentationQueueFamily = _device->GetPresentationQueueFamily();
        uint32_t queueFamilyIndices[] = { graphicsQueueFamily, presentationQueueFamily };
        if (graphicsQueueFamily != presentationQueueFamily) {
            printf("WARNING, all my tests were made in a GPU that the graphics and presentation queue families are equal. Proceed with caution.\n");
            swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            swapchainCreateInfo.queueFamilyIndexCount = 2;
            swapchainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
        }
        else {
            swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            swapchainCreateInfo.queueFamilyIndexCount = 0; // Optional
            swapchainCreateInfo.pQueueFamilyIndices = nullptr; // Optional
        }
        swapchainCreateInfo.preTransform = supportDetails.capabilities.currentTransform;
        swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        swapchainCreateInfo.presentMode = chosenPresentMode;
        swapchainCreateInfo.clipped = VK_TRUE;
        swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

        if (vkCreateSwapchainKHR(_device->GetDevice(), &swapchainCreateInfo, nullptr, &mSwapChain) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create swapchain!");
        }
        //now that we created the swap chain we get its images
        vkGetSwapchainImagesKHR(_device->GetDevice(), mSwapChain, &imageCount, nullptr);
        mSwapChainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(_device->GetDevice(), mSwapChain,
            &imageCount, mSwapChainImages.data());
        mImageFormat = chosenSurfaceFormat.format;
        mSwapChainImageViews.resize(mSwapChainImages.size());
        for (size_t i = 0; i < mSwapChainImages.size(); i++) {
            VkImageViewCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            createInfo.image = mSwapChainImages[i];
            createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            createInfo.format = mImageFormat;
            //default color mapping
            createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            //the image purpouse and which part of it to be accessed
            createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            createInfo.subresourceRange.baseMipLevel = 0;
            createInfo.subresourceRange.levelCount = 1;
            createInfo.subresourceRange.baseArrayLayer = 0;
            createInfo.subresourceRange.layerCount = 1;
            if (vkCreateImageView(_device->GetDevice(), &createInfo, nullptr,
                &mSwapChainImageViews[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to create image views!");
            }
        }
        SET_NAME(mSwapChain, VK_OBJECT_TYPE_SWAPCHAIN_KHR, "mainSwapChain");
        for (size_t i = 0; i < mSwapChainImages.size(); i++) {
            auto name1 = Concatenate("mainSwapChainImage", i);
            SET_NAME(mSwapChainImages[i], VK_OBJECT_TYPE_IMAGE, name1.c_str());
            auto name2 = Concatenate("mainSwapChainImageView", i);
            SET_NAME(mSwapChainImageViews[i], VK_OBJECT_TYPE_IMAGE_VIEW, name2.c_str());
        }
    }
    SwapChain::~SwapChain()
    {
        vkDestroySwapchainKHR(Device::gDevice->GetDevice(), mSwapChain, nullptr);
    }
}