#pragma once
#include <glm/glm.hpp>
#include <vulkan\vulkan.h>
#include <vector>
#include <optional>
#include <functional>
#include <optional>
#include "entities/light_uniform_buffer.h"
#include "entities/camera_uniform_buffer.h"
#include "utils/hash.h"
namespace vk {
    class DescriptorService;
    class RenderPass;
}
namespace entities
{
    class Pipeline;
    /// <summary>
    /// Holds the data for the color push constant.
    /// </summary>
    struct alignas(16) ColorPushConstantData {
        alignas(16)glm::vec4 color;
    };
    /// <summary>
    /// Creates a push constant range for the type T.
    /// </summary>
    /// <typeparam name="t"></typeparam>
    /// <returns></returns>
    template<typename T>
    VkPushConstantRange GetPushConstantRangeFor(VkShaderStageFlags stageFlags) {
        static VkPushConstantRange pushConstantRange = {};
        pushConstantRange.stageFlags = stageFlags;  // Specify the shader stage(s)
        pushConstantRange.offset = 0;                               // Offset within the push constant block
        pushConstantRange.size = sizeof(T); 
        return pushConstantRange;
    }
    VkPushConstantRange GetPushConstantRangeForObjectId();
    VkPipelineVertexInputStateCreateInfo GetVertexInputInfoForMesh();
    VkPipelineRasterizationStateCreateInfo GetBackfaceCullClockwiseRasterizationInfo();
    VkPipelineDepthStencilStateCreateInfo GetDefaultDepthStencil();
    VkPipelineColorBlendStateCreateInfo GetNoColorBlend();
    VkViewport GetViewportForSize(uint32_t w, uint32_t h);
    VkRect2D GetScissor(uint32_t w, uint32_t h);

    struct LightBuffers {
        AmbientLightUniformBuffer ambient;
        DirectionalLightUniformBuffer directionalLights;
    };
    typedef std::function<void(uintptr_t destAddr, VkCommandBuffer cmdBuffer, entities::Pipeline& pipeline, std::vector<VkDescriptorSet>& mCameraDescriptorSet, uint32_t currentFrame)> TCameraCallback;
    typedef std::function<LightBuffers()> TLightCallback;
    /// <summary>
    /// Load a shader from the compiled .spv to a shader module
    /// </summary>
    /// <param name="device"></param>
    /// <param name="name"></param>
    /// <returns></returns>
    VkShaderModule LoadShaderModule(VkDevice device, 
        const std::string& name);
    class Pipeline;
    /// <summary>
    /// Do not reuse builders. Create a new one for each pipeline.
    /// 
    /// It assumes that the input will always be triangles and not points/triangle strips/etc
    /// 
    /// It assumes one viewport and one scissor.
    /// </summary>
    class PipelineBuilder
    {
    public:
        PipelineBuilder(const std::string& name, vk::DescriptorService& descriptorService);
        /// <summary>
        /// Necessary: all pipelines need to know it's render pass
        /// </summary>
        /// <param name="rp"></param>
        /// <returns></returns>
        PipelineBuilder* SetRenderPass(vk::RenderPass* rp);
        /// <summary>
        /// Necessary: all pipelines need shaders
        /// </summary>
        /// <param name="vert"></param>
        /// <param name="frag"></param>
        /// <returns></returns>
        PipelineBuilder* SetShaderModules(VkShaderModule vert, VkShaderModule frag);
        /// <summary>
        /// Necessary: all pipelines needs it's root signatures
        /// </summary>
        /// <param name="layouts"></param>
        /// <returns></returns>
        PipelineBuilder* SetDescriptorSetLayouts(std::vector<VkDescriptorSetLayout> layouts);
        /// <summary>
        /// Optional: the push constants
        /// </summary>
        /// <param name="ranges"></param>
        /// <returns></returns>
        PipelineBuilder* SetPushConstantRanges(std::vector<VkPushConstantRange> ranges);
        /// <summary>
        /// Necessary
        /// </summary>
        /// <param name="info"></param>
        /// <returns></returns>
        PipelineBuilder* SetVertexInputStateInfo(VkPipelineVertexInputStateCreateInfo info);
        PipelineBuilder* SetDepthStencilStateInfo(VkPipelineDepthStencilStateCreateInfo info);
        PipelineBuilder* SetRasterizerStateInfo(VkPipelineRasterizationStateCreateInfo info);
        PipelineBuilder* SetColorBlending(VkPipelineColorBlendStateCreateInfo info);
        PipelineBuilder* SetViewport(VkViewport vp, VkRect2D scissor);
        PipelineBuilder* SetCameraCallback(TCameraCallback cbk);
        /// <summary>
        /// This callback is to provide light data for the pipeline. If the callback is defined then in Bind() 
        /// it'll be called and the light data passed to the gpu.
        /// </summary>
        /// <param name="cbk"></param>
        /// <returns></returns>
        PipelineBuilder* SetLightDataCallback(TLightCallback cbk);
        Pipeline* Build();

    private:
        std::vector<VkPushConstantRange> mPushConstantRanges{};
        std::vector<VkPipelineShaderStageCreateInfo> CreateShaderStageInfoForVertexAndFragment(VkShaderModule vs, 
            VkShaderModule fs);
        const std::string mName;
        Pipeline* mPipeline;
    };

    class Pipeline
    {
    public:
        Pipeline(vk::DescriptorService& descriptorService);
        ~Pipeline();
        VkPipelineLayout PipelineLayout()const {
            return mPipelineLayout;
        }
        hash_t Hash() { return mHash; }
        /// <summary>
        /// Bind the pipeline and if it has lightning data callback set applies lightning data.
        /// </summary>
        /// <param name="buffer"></param>
        /// <param name="currentFrame"></param>
        void Bind(VkCommandBuffer buffer, uint32_t currentFrame);

        template<typename T>
        void SetPushConstant(const T& value, 
            uint32_t id, 
            VkCommandBuffer cmdBuffer,
            VkShaderStageFlags stageFlags, uint32_t offset = 0) {
            vkCmdPushConstants(
                cmdBuffer,                   // Command buffer
                mPipelineLayout,                  // Pipeline layout
                stageFlags,      // Shader stage(s)
                offset,                               // Offset within the push constant block
                sizeof(T),                     // Size of the push constant data
                &value // Pointer to the data
            );
        }
        friend class PipelineBuilder;
        TCameraCallback mCameraCallback;
        vk::DescriptorService& mDescriptorService;
    private:
        
        std::optional<TLightCallback> mLightDataCallback;
        hash_t mHash;
        VkRect2D mScissor;
        VkViewport mViewport;
        VkPipelineColorBlendStateCreateInfo mColorBlending{};
        VkPipelineDepthStencilStateCreateInfo mDepthStencilInfo{};
        VkPipelineRasterizationStateCreateInfo mRasterizationStateInfo{};
        VkPipelineVertexInputStateCreateInfo mVertexInputStateInfo{};
        std::vector<VkDescriptorSetLayout> mLayouts;
        vk::RenderPass* mRenderPass = nullptr;
        VkPipeline mPipeline = VK_NULL_HANDLE;
        VkPipelineLayout mPipelineLayout = VK_NULL_HANDLE;
        void SetRenderPass(vk::RenderPass* rp);
        void SetShaderModules(VkShaderModule vert, VkShaderModule frag);
        VkShaderModule mVertexShader = VK_NULL_HANDLE;
        VkShaderModule mFragmentShader = VK_NULL_HANDLE;
    };
}