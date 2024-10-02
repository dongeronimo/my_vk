#include "framebuffer.h"
#include "render_pass.h"
#include <cassert>
#include "device.h"
#include <utils/concatenate.h>
#include "debug_utils.h"
namespace vk
{
    Framebuffer::Framebuffer(std::vector<VkImageView> colorAttachments, 
        VkImageView depthAttachment,
        VkExtent2D size, 
        RenderPass& renderPass, 
        const std::string& name): mName(name), mRenderPass(renderPass)
    {
        assert(colorAttachments.size() != 0);

        mFramebuffers.resize(colorAttachments.size());
        for (size_t i = 0; i < colorAttachments.size(); i++) {
            VkImageView colorAttachment = colorAttachments[i];
            std::vector<VkImageView> attachments{
                colorAttachment, depthAttachment 
            };
            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = renderPass.GetRenderPass();
            framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
            framebufferInfo.pAttachments = attachments.data();
            framebufferInfo.width = size.width;
            framebufferInfo.height = size.height;
            framebufferInfo.layers = 1;

            if (vkCreateFramebuffer(vk::Device::gDevice->GetDevice(), 
                &framebufferInfo, nullptr,
                &mFramebuffers[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to create framebuffer!");
            }
            auto _name = Concatenate(mName, "Framebuffer ", i);
            SET_NAME(mFramebuffers[i], VK_OBJECT_TYPE_FRAMEBUFFER, _name.c_str());
        }
    }
    Framebuffer::Framebuffer(std::vector<VkImageView> colorAttachments, VkExtent2D size, RenderPass& renderPass, const std::string& name)
        :mName(name), mRenderPass(renderPass)
    {
        assert(colorAttachments.size() != 0);
        mFramebuffers.resize(colorAttachments.size());
        for (size_t i = 0; i < colorAttachments.size(); i++) {
            VkImageView colorAttachment = colorAttachments[i];
            std::vector<VkImageView> attachments{
                colorAttachment
            };
            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = renderPass.GetRenderPass();
            framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
            framebufferInfo.pAttachments = attachments.data();
            framebufferInfo.width = size.width;
            framebufferInfo.height = size.height;
            framebufferInfo.layers = 1;

            if (vkCreateFramebuffer(vk::Device::gDevice->GetDevice(),
                &framebufferInfo, nullptr,
                &mFramebuffers[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to create framebuffer!");
            }
            auto _name = Concatenate(mName, "Framebuffer ", i);
            SET_NAME(mFramebuffers[i], VK_OBJECT_TYPE_FRAMEBUFFER, _name.c_str());
        }
    }
}
