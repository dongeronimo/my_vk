#pragma once
#include <map>
#include <vector>
#include <vulkan/vulkan.h>
#include "utils/hash.h"
namespace vk
{
   template<typename t>
    VkDeviceSize CalculateAlignedSize() {
        constexpr size_t modelDataSize = sizeof(t);
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(vk::Instance::gInstance->GetPhysicalDevice(), &deviceProperties);
        const VkDeviceSize minUniformBufferOffsetAlignment = deviceProperties.limits.minUniformBufferOffsetAlignment;
        const VkDeviceSize alignedModelDataSize = (modelDataSize + minUniformBufferOffsetAlignment - 1) & ~(minUniformBufferOffsetAlignment - 1);
        return alignedModelDataSize;
    }

    template<typename t>
    uintptr_t CalculateDynamicOffset(uint32_t currentFrame, uint32_t maxNumber, uint32_t id) {
        const VkDeviceSize alignedModelDataSize = CalculateAlignedSize<t>();
        VkDeviceSize bufferOffset = (currentFrame * maxNumber + id) * alignedModelDataSize;
        return bufferOffset;
    }
    class SamplerService;
    class ImageService;
    const uint32_t CAMERA_SET = 0;
    const uint32_t MODEL_MATRIX_SET = 1;
    const uint32_t LIGHT_SET = 2;
    const uint32_t MAX_NUMBER_OF_OBJECTS = 10000 * MAX_FRAMES_IN_FLIGHT;
    const uint32_t MAX_NUMBER_OF_CAMERAS = MAX_FRAMES_IN_FLIGHT;
    const std::string LIGHTNING_LAYOUT_NAME = "LightningDescriptorSet";
    const std::string CAMERA_LAYOUT_NAME = "CameraDataDescriptorSet";
    const std::string MODEL_MATRIX_LAYOUT_NAME = "ModelMatrixDescriptorSet";
    const std::string SAMPLER_LAYOUT_NAME = "SamplerDescriptorSet";
    const std::string FAKE_SHADOW_MAP_SAMPLERS = "FakeShadowMapSamplers";
    /// <summary>
    /// Manages descriptor set layouts, descriptor pools and descriptor sets.
    /// 
    /// </summary>
    class DescriptorService {
    public:
        
        DescriptorService(SamplerService& samplerService,
            ImageService& imageService);
        ~DescriptorService();
        VkDescriptorSetLayout DescriptorSetLayout(const std::string& name)const;
        std::vector<VkDescriptorSet> DescriptorSet(const std::string& name,
            uint32_t idx)const;
        std::vector<VkDescriptorSet> DescriptorSet(const std::string& name)const;
        std::vector<uintptr_t> DescriptorSetsBuffersAddrs(const std::string& name,
            uint32_t idx, bool alredyAddedBaseAddress = false)const;
        uintptr_t ModelMatrixDescriptorSetDynamicOffset( uint32_t idx);

        uintptr_t ModelMatrixBaseAddress() {
            return mBaseAddesses.at(utils::Hash(MODEL_MATRIX_LAYOUT_NAME));
        }
    private:
        SamplerService& mSamplerService;
        struct DescriptorSetBuffer {
            VkBuffer mBuffer;
            VkDeviceMemory mMemory;
            std::uintptr_t mBasePointer;
        };
        /// <summary>
        /// Creates a buffer, it's memory and the map to that memory
        /// </summary>
        /// <param name="numElements"></param>
        /// <param name="sizeOfEachElement"></param>
        /// <param name="usage"></param>
        /// <returns></returns>
        static DescriptorSetBuffer CreateBuffer(
            uint32_t numElements, VkDeviceSize sizeOfEachElement, VkMemoryPropertyFlags memoryType, VkBufferUsageFlags usage, uintptr_t& baseAddress);
        /// <summary>
        /// Holds the descriptor set layouts;
        /// </summary>
        std::map<hash_t, VkDescriptorSetLayout> mLayouts;
        /// <summary>
        /// Holds the descriptor pools
        /// </summary>
        std::map<hash_t, VkDescriptorPool> mDescriptorPools;
        /// <summary>
        /// Holds the descriptor sets
        /// </summary>
        std::map<hash_t, const std::vector<VkDescriptorSet>> mDescriptorSets;
        /// <summary>
        /// Holds the buffer of each descriptor set
        /// </summary>
        std::map<hash_t, DescriptorSetBuffer> mDescriptorSetBuffers;
        /// <summary>
        /// Store the addresses of the buffers or dynamic offsets, 
        /// </summary>
        std::map<hash_t, const std::vector<std::uintptr_t>> mDescriptorSetsBuffersOffsets;
        std::map<hash_t, const uintptr_t> mBaseAddesses;

        VkDescriptorSetLayout CreateDescriptorSetLayoutForCamera();
        VkDescriptorPool CreateDescriptorPoolForCamera();
        const std::vector<VkDescriptorSet> CreateDescriptorSetForCamera();

        VkDescriptorSetLayout CreateDescriptorSetLayoutForObject();
        VkDescriptorPool CreateDescriptorPoolForModelMatrix();
        const std::vector<VkDescriptorSet> CreateDescriptorSetForModelMatrix();

        VkDescriptorSetLayout CreateDescriptorSetLayoutForSampler();
        VkDescriptorPool CreateDescriptorPoolForSampler();

        std::vector<VkDescriptorSet> GenerateSamplerDescriptorSetsForTexture(VkDescriptorPool samplerDescriptorPool,
            VkDescriptorSetLayout samplerDescriptorSetLayout,
            VkImageView imageView,
            VkSampler sampler,
            const std::string& name);

        void CreateDescriptorSetLayoutForLight();
        void CreateDescriptorPoolForLight();
        void CreateDescriptorSetForLight();

        void CreateDescriptorSetLayoutForFakeShadowBuffer();
        void CreateDescriptorPoolForFakeShadowBuffer();
        void CreateDescriptorSetForFakeShadowBuffer(std::vector<VkImageView> colorImageViews, VkImageView depthImageViews);
    };
}