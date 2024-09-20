#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <string>
namespace vk
{
    class RenderPass;
    class Framebuffer {
    public:
        Framebuffer(std::vector<VkImageView> colorAttachments, 
            std::vector<VkImageView> depthAttachments,
            VkExtent2D size,
            RenderPass& renderPass,
            const std::string& name);
        Framebuffer(std::vector<VkImageView> colorAttachments,
            VkExtent2D size,
            RenderPass& renderPass,
            const std::string& name);
        const std::string mName;
    private:
        std::vector<VkFramebuffer> mFramebuffers;
    };
}