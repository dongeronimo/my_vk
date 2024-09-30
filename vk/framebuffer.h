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
            std::vector<VkImageView> depthAttachment,
            VkExtent2D size,
            RenderPass& renderPass,
            const std::string& name);
        Framebuffer(std::vector<VkImageView> colorAttachments,
            VkExtent2D size,
            RenderPass& renderPass,
            const std::string& name);
        const std::string mName;
        VkFramebuffer& GetFramebuffer(uint32_t idx)  {
            return mFramebuffers[idx];
        }
    private:
        std::vector<VkFramebuffer> mFramebuffers;
    };
}