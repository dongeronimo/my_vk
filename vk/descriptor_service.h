#pragma once
#include <map>
#include <vector>
#include <vulkan/vulkan.h>
namespace vk
{
    class SamplerService;
    class ImageService;
    const uint32_t CAMERA_SET = 0;
    const uint32_t MODEL_MATRIX_SET = 1;
    const uint32_t MAX_NUMBER_OF_OBJECTS = 10000 * MAX_FRAMES_IN_FLIGHT;
    const uint32_t MAX_NUMBER_OF_CAMERAS = MAX_FRAMES_IN_FLIGHT;
    const std::string CAMERA_LAYOUT_NAME = "CameraDataDescriptorSet";
    const std::string MODEL_MATRIX_LAYOUT_NAME = "ModelMatrixDescriptorSet";
    const std::string SAMPLER_LAYOUT_NAME = "SamplerDescriptorSet";
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
        std::vector<uintptr_t> DescriptorSetsBuffersAddrs(const std::string& name,
            uint32_t idx)const;
        uintptr_t DescriptorSetDynamicOffset(const std::string& name, uint32_t idx);
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
        std::map<size_t, VkDescriptorSetLayout> mLayouts;
        /// <summary>
        /// Holds the descriptor pools
        /// </summary>
        std::map<size_t, VkDescriptorPool> mDescriptorPools;
        /// <summary>
        /// Holds the descriptor sets
        /// </summary>
        std::map<size_t, const std::vector<VkDescriptorSet>> mDescriptorSets;
        /// <summary>
        /// Holds the buffer of each descriptor set
        /// </summary>
        std::map<size_t, DescriptorSetBuffer> mDescriptorSetBuffers;
        /// <summary>
        /// Store the addresses of the buffers or dynamic offsets, 
        /// </summary>
        std::map<size_t, const std::vector<std::uintptr_t>> mDescriptorSetsBuffersOffsets;
        std::map<size_t, const uintptr_t> mBaseAddesses;

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
    };
}