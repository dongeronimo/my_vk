#pragma once
#include <vulkan/vulkan.h>
#include <string>
#include <vector>

namespace vk
{
    class RenderPass
    {
    public:
        RenderPass(VkFormat colorFormat,
            const std::string& name);
        RenderPass(VkFormat depthFormat, VkFormat colorFormat, 
            const std::string& name);
        VkRenderPass GetRenderPass()const { return mRenderPass; }
        ~RenderPass();
    private:
        VkRenderPass mRenderPass = VK_NULL_HANDLE;
    };
}