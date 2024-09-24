#pragma once
#include <vulkan/vulkan.h>
#include <string>
#include <vector>
#include <glm/glm.hpp>
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
        void BeginRenderPass(glm::vec4 color,
            VkClearDepthStencilValue depth,
            VkFramebuffer framebuffer,
            VkExtent2D extent, 
            VkCommandBuffer cmdBuffer);
        void EndRenderPass(VkCommandBuffer cmdBuffer);
        ~RenderPass();
    private:
        VkRenderPass mRenderPass = VK_NULL_HANDLE;
    };
}