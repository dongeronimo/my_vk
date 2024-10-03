#pragma once
#include <vulkan/vulkan.h>
#include <string>
#include <functional>
#include <vector>
#include <glm/glm.hpp>
#include <optional>
namespace vk
{
    class RenderPass
    {
    public:
        typedef std::function<void(RenderPass* rp, VkCommandBuffer cmdBuffer, uint32_t currentFrame)> TOnRenderPassBegin;
        typedef std::function<void(RenderPass* rp,VkCommandBuffer cmdBuffer, uint32_t currentFrame)> TOnRenderPassEnd;
        /// <summary>
        /// A render pass with only color buffer
        /// </summary>
        /// <param name="colorFormat"></param>
        /// <param name="name"></param>
        RenderPass(VkFormat colorFormat,
            const std::string& name,
            VkImageLayout finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
        /// <summary>
        /// A render pass with color and depth buffer
        /// </summary>
        /// <param name="depthFormat"></param>
        /// <param name="colorFormat"></param>
        /// <param name="name"></param>
        RenderPass(VkFormat depthFormat, VkFormat colorFormat, 
            const std::string& name);

        VkRenderPass GetRenderPass()const { return mRenderPass; }
        void BeginRenderPass(glm::vec4 color,
            VkClearDepthStencilValue depth,
            VkFramebuffer framebuffer,
            VkExtent2D extent, 
            VkCommandBuffer cmdBuffer, 
            uint32_t currentFrame);
        void EndRenderPass(VkCommandBuffer cmdBuffer, uint32_t currentFrame);
        ~RenderPass();
        std::optional<TOnRenderPassEnd> mOnRenderPassEndCallback;
        std::optional<TOnRenderPassBegin> mOnRenderPassBeginCallback;
        const std::string mName;
        /// <summary>
        /// Builds a render pass for shadow mapping, it'll have one depth buffer and no color buffer.
        /// </summary>
        /// <param name="name"></param>
        /// <returns></returns>
        static RenderPass* RenderPassForShadowMapping(const std::string& name);

        static RenderPass* FakeShadowMapPass(const std::string& name);
    private:
        RenderPass(const std::string& name) :mName(name) {};
        VkRenderPass mRenderPass = VK_NULL_HANDLE;
    };
}