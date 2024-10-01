#include "pipeline.h"
#include <io/asset-paths.h>
#include <fstream>
#include <utils/concatenate.h>
#include <vk/debug_utils.h>
#include <vk/device.h>
#include <vk/render_pass.h>
#include <vk/descriptor_service.h>
#include "mesh.h"
#include <utils/hash.h>
namespace entities
{
    VkPushConstantRange GetPushConstantRangeForObjectId()
    {
        static VkPushConstantRange pushConstantRange = {};
        pushConstantRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;  // Specify the shader stage(s)
        pushConstantRange.offset = 0;                               // Offset within the push constant block
        pushConstantRange.size = sizeof(uint32_t); //The id is just an int
        return pushConstantRange;
    }
    VkShaderModule LoadShaderModule(VkDevice device, const std::string& name)
    {
        auto path = io::CalculatePathForShader(name);
        std::ifstream file(path, std::ios::ate | std::ios::binary);
        if (!file.is_open())
            throw std::runtime_error("failed to open file!");
        size_t fileSize = (size_t)file.tellg();
        std::vector<char> binaryCode(fileSize);
        file.seekg(0);
        file.read(binaryCode.data(), fileSize);
        file.close();
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = binaryCode.size();
        createInfo.pCode = reinterpret_cast<const uint32_t*>(binaryCode.data());
        VkShaderModule shaderModule;
        if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
            auto errMsg = Concatenate("failed to create shader module: ", name);
            throw std::runtime_error(errMsg);
        }
        SET_NAME(shaderModule, VK_OBJECT_TYPE_SHADER_MODULE, name.c_str());
        return shaderModule;
    }
    
    Pipeline::Pipeline(vk::DescriptorService& descriptorService):
        mDescriptorService(descriptorService)
    {
    }

    Pipeline::~Pipeline()
    {
        auto d = vk::Device::gDevice->GetDevice();
        vkDestroyPipelineLayout(d, mPipelineLayout, nullptr);
        vkDestroyPipeline(d, mPipeline, nullptr);
        vkDestroyShaderModule(d, mFragmentShader, nullptr);
        vkDestroyShaderModule(d, mVertexShader, nullptr);
    }

    void Pipeline::Bind(VkCommandBuffer buffer, uint32_t currentFrame)
    {
        
        vkCmdBindPipeline(buffer,VK_PIPELINE_BIND_POINT_GRAPHICS, mPipeline);
        if (mLightDataCallback) {
            entities::LightBuffers lightBuffers = (*mLightDataCallback)();
            uintptr_t ambientAddr = mDescriptorService.DescriptorSetsBuffersAddrs(Concatenate(vk::LIGHTNING_LAYOUT_NAME, "Ambient"), 0)[currentFrame];
            uintptr_t directionalAddr = mDescriptorService.DescriptorSetsBuffersAddrs(Concatenate(vk::LIGHTNING_LAYOUT_NAME, "Directional"), 0)[currentFrame];
            memcpy(reinterpret_cast<void*>(ambientAddr), &lightBuffers.ambient, sizeof(entities::AmbientLightUniformBuffer));
            memcpy(reinterpret_cast<void*>(directionalAddr), &lightBuffers.directionalLights, sizeof(entities::DirectionalLightUniformBuffer));
            VkDescriptorSet descriptorSet = mDescriptorService.DescriptorSet(vk::LIGHTNING_LAYOUT_NAME)[currentFrame];
            vkCmdBindDescriptorSets(
                buffer,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                PipelineLayout(),
                vk::LIGHT_SET,
                1,
                &descriptorSet,
                0,
                nullptr
            );
        }
    }

    void Pipeline::SetRenderPass(vk::RenderPass* rp)
    {
        this->mRenderPass = rp;
    }

    void Pipeline::SetShaderModules(VkShaderModule vert, VkShaderModule frag)
    {
        this->mVertexShader = vert;
        this->mFragmentShader = frag;
    }

    PipelineBuilder::PipelineBuilder(const std::string& name, vk::DescriptorService& descriptorService)
        :mName(name)
    {
        this->mPipeline = new Pipeline(descriptorService);
    }

    PipelineBuilder* PipelineBuilder::SetRenderPass(vk::RenderPass* rp)
    {
        mPipeline->SetRenderPass(rp);
        return this;
    }

    PipelineBuilder* PipelineBuilder::SetShaderModules(VkShaderModule vert, VkShaderModule frag)
    {
        mPipeline->SetShaderModules(vert, frag);
        return this;
    }

    PipelineBuilder* PipelineBuilder::SetDescriptorSetLayouts(std::vector<VkDescriptorSetLayout> layouts)
    {
        mPipeline->mLayouts = layouts;
        return this;
    }

    PipelineBuilder* PipelineBuilder::SetPushConstantRanges(std::vector<VkPushConstantRange> ranges)
    {
        mPushConstantRanges = ranges;
        return this;
    }

    PipelineBuilder* PipelineBuilder::SetVertexInputStateInfo(VkPipelineVertexInputStateCreateInfo info)
    {
        mPipeline->mVertexInputStateInfo = info;
        return this;
    }

    PipelineBuilder* PipelineBuilder::SetDepthStencilStateInfo(VkPipelineDepthStencilStateCreateInfo info)
    {
        mPipeline->mDepthStencilInfo = info;
        return this;
    }

    PipelineBuilder* PipelineBuilder::SetRasterizerStateInfo(VkPipelineRasterizationStateCreateInfo info)
    {
        mPipeline->mRasterizationStateInfo = info;
        return this;
    }

    PipelineBuilder* PipelineBuilder::SetColorBlending(VkPipelineColorBlendStateCreateInfo info)
    {
        mPipeline->mColorBlending = info;
        return this;
    }

    PipelineBuilder* PipelineBuilder::SetViewport(VkViewport vp, VkRect2D scissor)
    {
        mPipeline->mViewport = vp;
        mPipeline->mScissor = scissor;
        return this;
    }

    PipelineBuilder* PipelineBuilder::SetLightDataCallback(TLightCallback cbk)
    {
        mPipeline->mLightDataCallback = cbk;
        return this;
    }

    Pipeline* PipelineBuilder::Build()
    {
        assert(mPipeline->mRenderPass != nullptr);
        assert(mPipeline->mVertexShader != VK_NULL_HANDLE);
        assert(mPipeline->mFragmentShader != VK_NULL_HANDLE);
        assert(mPipeline->mVertexInputStateInfo.sType == VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO);
        assert(mPipeline->mRasterizationStateInfo.sType == VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO);
        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;
        viewportState.pViewports = &mPipeline->mViewport;
        viewportState.pScissors = &mPipeline->mScissor;
        //input description: the geometry input will be triangles
        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;
        //multisampling TODO pipeline: Disabled for now
        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        ////////////THE SHADER STAGES/////////////////
        std::vector<VkPipelineShaderStageCreateInfo> shaderStages = CreateShaderStageInfoForVertexAndFragment(
            mPipeline->mVertexShader, mPipeline->mFragmentShader);
        //the dynamic states - they will have to set up in draw time
        std::vector<VkDynamicState> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT,VK_DYNAMIC_STATE_SCISSOR };
        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicState.pDynamicStates = dynamicStates.data();
        ////////////THE PIPELINE LAYOUT/////////////
        //pipeline layout, to pass data to the shaders
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(mPipeline->mLayouts.size());
        pipelineLayoutInfo.pSetLayouts = mPipeline->mLayouts.data();        
        //will we use push constants? Depends whether SetPushConstantRanges has been called.
        if (mPushConstantRanges.size() > 0) {
            pipelineLayoutInfo.pushConstantRangeCount = mPushConstantRanges.size();
            pipelineLayoutInfo.pPushConstantRanges = mPushConstantRanges.data();
        }
        else {
            pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
            pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional
        }
        if (vkCreatePipelineLayout(vk::Device::gDevice->GetDevice(), &pipelineLayoutInfo,
            nullptr, &mPipeline->mPipelineLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create pipeline layout!");
        }
        auto plname = Concatenate(mName, "PipelineLayout");
        SET_NAME(mPipeline->mPipelineLayout,
            VK_OBJECT_TYPE_PIPELINE_LAYOUT, plname.c_str());
        ////////////THE PIPELINE///////////
        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
        pipelineInfo.pStages = shaderStages.data();    //add the shader stages
        //set the states
        pipelineInfo.pVertexInputState = &mPipeline->mVertexInputStateInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &mPipeline->mRasterizationStateInfo;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pDepthStencilState = &mPipeline->mDepthStencilInfo;
        pipelineInfo.pColorBlendState = &mPipeline->mColorBlending;
        pipelineInfo.pDynamicState = &dynamicState;
        //link the pipeline layout to the pipeline
        pipelineInfo.layout = mPipeline->mPipelineLayout;
        //link the render pass to the pipeline
        pipelineInfo.renderPass = mPipeline->mRenderPass->GetRenderPass();
        pipelineInfo.subpass = 0;
        if (vkCreateGraphicsPipelines(vk::Device::gDevice->GetDevice(), VK_NULL_HANDLE,
            1,//number of pipelines 
            &pipelineInfo, //list of pipelines (only one) 
            nullptr, &mPipeline->mPipeline) != VK_SUCCESS) {
            throw std::runtime_error("failed to create graphics pipeline!");
        }
        SET_NAME(mPipeline->mPipeline, VK_OBJECT_TYPE_PIPELINE, mName.c_str());
        mPipeline->mHash = utils::Hash(mName);
        auto result = mPipeline;
        mPipeline = nullptr;
        return result;
    }

    std::vector<VkPipelineShaderStageCreateInfo> PipelineBuilder::CreateShaderStageInfoForVertexAndFragment(VkShaderModule vs, VkShaderModule fs)
    {
        //description of the shader stages
        VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
        vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertShaderStageInfo.module = vs;
        vertShaderStageInfo.pName = "main";
        VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
        fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragShaderStageInfo.module = fs;
        fragShaderStageInfo.pName = "main";
        std::vector<VkPipelineShaderStageCreateInfo> shaderStages = {
            vertShaderStageInfo,
            fragShaderStageInfo };
        return shaderStages;
    }

    VkPipelineVertexInputStateCreateInfo GetVertexInputInfoForMesh()
    {
        static VkPipelineVertexInputStateCreateInfo vertexInputInfo;
        static auto bindingDescription = entities::Mesh::BindingDescription();
        static auto attributeDescriptions = entities::Mesh::AttributeDescription();
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
        vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
        vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
        return vertexInputInfo;
    }

    VkPipelineRasterizationStateCreateInfo GetBackfaceCullClockwiseRasterizationInfo()
    {
        static VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE; //only relevant when doing shadow maps
        rasterizer.rasterizerDiscardEnable = VK_FALSE; //if true the geometry will never pass thru the rasterizer stage
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;
        rasterizer.lineWidth = 1.0f;
        return rasterizer;
    }

    VkPipelineDepthStencilStateCreateInfo GetDefaultDepthStencil()
    {
        static VkPipelineDepthStencilStateCreateInfo depthStencil{};
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = VK_TRUE;
        depthStencil.depthWriteEnable = VK_TRUE;
        depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.stencilTestEnable = VK_FALSE;  
        return depthStencil;
    }

    VkPipelineColorBlendStateCreateInfo GetNoColorBlend()
    {
        static VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_FALSE; //TODO: no color blending for now
        //  global
        static VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE; //TODO: no color blending for now
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;
        return colorBlending;
    }

    VkViewport GetViewportForSize(uint32_t w, uint32_t h)
    {
        // Define the viewport and scissor
        VkViewport viewport = {};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(w);
        viewport.height = static_cast<float>(h);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        return viewport;

    }

    VkRect2D GetScissor(uint32_t w, uint32_t h)
    {
        VkExtent2D sz{ w,h };
        VkRect2D scissor = {};
        scissor.offset = { 0, 0 };
        scissor.extent = sz;
        return scissor;
    }

}