#include "render_pass.h"
#include <array>
#include "device.h"
#include "debug_utils.h"
namespace vk {
    RenderPass* RenderPass::RenderPassForShadowMapping(const std::string& name)
    {
        VkDevice device = vk::Device::gDevice->GetDevice();
        assert(name.size() > 0);
        VkAttachmentDescription depthAttachment = {};
        depthAttachment.format = VK_FORMAT_D32_SFLOAT;  // e.g., VK_FORMAT_D32_SFLOAT
        depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;  // Typically no MSAA for shadow maps
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;  // Clear depth at start
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;  // Store depth for later sampling
        depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;  // No stencil use
        depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;  // Starting layout
        depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL; 
        VkAttachmentReference depthAttachmentRef = {};
        depthAttachmentRef.attachment = 0;  // Index of the depth attachment
        depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;  // Layout for depth writing
        VkSubpassDescription subpass = {};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 0;  // No color attachments
        subpass.pDepthStencilAttachment = &depthAttachmentRef;  // Depth attachment reference
        VkSubpassDependency dependency = {};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;  // This subpass
        dependency.srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dependency.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        dependency.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        dependency.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        dependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
        VkRenderPassCreateInfo renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = 1;  // Just the depth attachment
        renderPassInfo.pAttachments = &depthAttachment;
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;
        RenderPass* renderPass = new RenderPass(name);
        if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass->mRenderPass) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create shadow map render pass!");
        }
        SET_NAME(renderPass->mRenderPass, VK_OBJECT_TYPE_RENDER_PASS, name.c_str());
        return renderPass;
    }
    RenderPass::RenderPass(VkFormat colorFormat, const std::string& name,
        VkImageLayout finalLayout):mName(name)
    {
        assert(mName.size() > 0);
        //the image that'll hold the result
        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = colorFormat;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;//clear the values to a constant at the start
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; //store the rendered result in memory for later uses
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;//not using the stencil buffer
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = finalLayout;//image presented in the swap chain
        //the reference to the image attachment that'll hold the result
        VkAttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        //the render subpass
        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;
        //the actual render pass, composed of image attachments and subpasses
        std::array<VkAttachmentDescription, 1> attachments = {colorAttachment};
        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        renderPassInfo.pAttachments = attachments.data();
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        //the render pass depends upon the previous render pass to run
        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        //wait until the image is attached
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;

        if (vkCreateRenderPass(vk::Device::gDevice->GetDevice(), &renderPassInfo, nullptr,
            &mRenderPass) != VK_SUCCESS) {
            throw std::runtime_error("failed to create render pass!");
        }
        SET_NAME(mRenderPass, VK_OBJECT_TYPE_RENDER_PASS, name.c_str());
    }
    RenderPass::RenderPass(VkFormat depthFormat, VkFormat colorFormat,
        const std::string& name):
        mName(name)
    {
        assert(mName.size() > 0);
        VkAttachmentDescription depthAttachment{};
        depthAttachment.format = depthFormat;
        depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        VkAttachmentReference depthAttachmentRef{};
        depthAttachmentRef.attachment = 1;
        depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        //the image that'll hold the result
        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = colorFormat;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;//clear the values to a constant at the start
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; //store the rendered result in memory for later uses
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;//not using the stencil buffer
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;//image presented in the swap chain
        //the reference to the image attachment that'll hold the result
        VkAttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        //the render subpass
        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;
        subpass.pDepthStencilAttachment = &depthAttachmentRef;
        //the actual render pass, composed of image attachments and subpasses
        std::array<VkAttachmentDescription, 2> attachments = {
            colorAttachment,
            depthAttachment };
        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        renderPassInfo.pAttachments = attachments.data();
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        //the render pass depends upon the previous render pass to run
        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        //wait until the image is attached
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;

        if (vkCreateRenderPass(vk::Device::gDevice->GetDevice(), &renderPassInfo, nullptr, 
            &mRenderPass) != VK_SUCCESS) {
            throw std::runtime_error("failed to create render pass!");
        }
        SET_NAME(mRenderPass, VK_OBJECT_TYPE_RENDER_PASS, name.c_str());
    }
    void RenderPass::BeginRenderPass(glm::vec4 color, 
        VkClearDepthStencilValue depth, VkFramebuffer framebuffer,
        VkExtent2D extent, VkCommandBuffer cmdBuffer, uint32_t currentFrame)
    {
        if (mOnRenderPassBeginCallback) {
            (*mOnRenderPassBeginCallback)(this, cmdBuffer, currentFrame);
        }
        //begin the render pass of the render pass
        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = mRenderPass;
        renderPassInfo.framebuffer = framebuffer;
        renderPassInfo.renderArea.offset = { 0, 0 };
        renderPassInfo.renderArea.extent = extent;
        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color = { {color.r, color.g, color.b, color.a } };
        clearValues[1].depthStencil = depth;
        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();
        vkCmdBeginRenderPass(cmdBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        vk::SetMark({ 0.0f, 0.8f, 4.0f, 1.0f }, mName, cmdBuffer);
    }
    void RenderPass::EndRenderPass(VkCommandBuffer cmdBuffer, uint32_t currentFrame)
    {
        vk::EndMark(cmdBuffer);
        vkCmdEndRenderPass(cmdBuffer);
        if (mOnRenderPassEndCallback) {
            (*mOnRenderPassEndCallback)(this, cmdBuffer, currentFrame);
        }
    }
    RenderPass::~RenderPass()
    {
        vkDestroyRenderPass(vk::Device::gDevice->GetDevice(),
            mRenderPass, nullptr);
    }

}
