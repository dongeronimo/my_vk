#include "vk\descriptor_service.h"
#include "vk\device.h"
#include "vk\debug_utils.h"
#include "utils\hash.h"
#include "utils/vk_utils.h"
#include "utils/concatenate.h"
#include <entities/camera_uniform_buffer.h>
#include <entities/model_matrix_uniform_buffer.h>
#include "samplers_service.h"
#include "image_service.h"
#include "vk\instance.h"
#include "entities\light_uniform_buffer.h"
const uint32_t MODEL_POOL_SIZE = 10000;
const uint32_t SAMPLER_POOL_SIZE = 1000;

namespace vk
{
    DescriptorService::DescriptorService(SamplerService& samplerService,
        ImageService& imageService, std::vector<VkImageView> fakeShadowMapColorBuffers, VkImageView fakeShadowMapDepthBuffer)
        :mSamplerService(samplerService)
    {
        mLayouts.insert({ utils::Hash(CAMERA_LAYOUT_NAME), CreateDescriptorSetLayoutForCamera() });
        mDescriptorPools.insert({ utils::Hash(CAMERA_LAYOUT_NAME), CreateDescriptorPoolForCamera() });
        mDescriptorSets.insert({ utils::Hash(CAMERA_LAYOUT_NAME), CreateDescriptorSetForCamera() });
        
        mLayouts.insert({ utils::Hash(MODEL_MATRIX_LAYOUT_NAME), CreateDescriptorSetLayoutForObject() });
        mDescriptorPools.insert({ utils::Hash(MODEL_MATRIX_LAYOUT_NAME), CreateDescriptorPoolForModelMatrix() });
        mDescriptorSets.insert({ utils::Hash(MODEL_MATRIX_LAYOUT_NAME), CreateDescriptorSetForModelMatrix() });
        
        mLayouts.insert({ utils::Hash(SAMPLER_LAYOUT_NAME), CreateDescriptorSetLayoutForSampler() });
        mDescriptorPools.insert({ utils::Hash(SAMPLER_LAYOUT_NAME), CreateDescriptorPoolForSampler() });
        // Remember that the descriptor set varies with the sampler and the imageview
    
        //create the descriptor sets for each image in the image service.
        const VkSampler mySampler = samplerService.GetSampler(LINEAR_REPEAT_NORMALIZED_NONMIPMAP_SAMPLER);
        const entities::Image& blackBrick = imageService.GetImage("blackBrick.png");
        mDescriptorSets.insert({ 
            utils::Hash("blackBrick.png"),
            GenerateSamplerDescriptorSetsForTexture(
                mDescriptorPools.at(utils::Hash(SAMPLER_LAYOUT_NAME)),
                mLayouts.at(utils::Hash(SAMPLER_LAYOUT_NAME)),
                blackBrick.mImageView,
                mySampler,
                "blackBrick.png")
        });
        const entities::Image& brick = imageService.GetImage("brick.png");
        mDescriptorSets.insert({
            utils::Hash("brick.png"),
            GenerateSamplerDescriptorSetsForTexture(
                mDescriptorPools.at(utils::Hash(SAMPLER_LAYOUT_NAME)),
                mLayouts.at(utils::Hash(SAMPLER_LAYOUT_NAME)),
                brick.mImageView,
                mySampler,
                "brick.png")
            });

        const entities::Image& floor01 = imageService.GetImage("floor01.jpg");
        mDescriptorSets.insert({
            utils::Hash("floor01.jpg"),
            GenerateSamplerDescriptorSetsForTexture(
                mDescriptorPools.at(utils::Hash(SAMPLER_LAYOUT_NAME)),
                mLayouts.at(utils::Hash(SAMPLER_LAYOUT_NAME)),
                brick.mImageView,
                mySampler,
                "floor01.jpg")
                    });
        CreateDescriptorSetLayoutForLight();
        CreateDescriptorPoolForLight();
        CreateDescriptorSetForLight();

        CreateDescriptorSetLayoutForFakeShadowBuffer();
        CreateDescriptorPoolForFakeShadowBuffer();
        CreateDescriptorSetForFakeShadowBuffer(fakeShadowMapColorBuffers, fakeShadowMapDepthBuffer);
    
    }
    DescriptorService::~DescriptorService()
    {
        for (auto& kv : mLayouts) {
            vkDestroyDescriptorSetLayout(Device::gDevice->GetDevice(), kv.second, nullptr);
        }
        mLayouts.clear();
        
        for (auto& kv : mDescriptorPools) {
            vkDestroyDescriptorPool(Device::gDevice->GetDevice(), kv.second, nullptr);
        }
        mDescriptorPools.clear();

        for (auto& kv : mDescriptorSetBuffers) {
            vkUnmapMemory(Device::gDevice->GetDevice(), kv.second.mMemory);
            vkFreeMemory(Device::gDevice->GetDevice(), kv.second.mMemory, nullptr);
            vkDestroyBuffer(Device::gDevice->GetDevice(), kv.second.mBuffer, nullptr);
        }
        mDescriptorSetBuffers.clear();
    }
    VkDescriptorSetLayout DescriptorService::DescriptorSetLayout(const std::string& name) const
    {
        auto hash = utils::Hash(name);
        assert(mLayouts.count(hash) > 0);
        return mLayouts.at(hash);
    }
    std::vector<VkDescriptorSet> DescriptorService::DescriptorSet(const std::string& name,
        uint32_t idx) const
    {
        //Get the descriptor set bucket that i want
        auto hash = utils::Hash(name);
        assert(mDescriptorSets.count(hash) > 0);
        auto allDS = mDescriptorSets.at(hash);
        //now that only the descriptor set that i want
        std::vector<VkDescriptorSet> slice(MAX_FRAMES_IN_FLIGHT);
        for (auto i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            slice[i] = allDS[idx * MAX_FRAMES_IN_FLIGHT + i];
        }
        return slice;
    }
    std::vector<VkDescriptorSet> DescriptorService::DescriptorSet(const std::string& name) const
    {
        auto hash = utils::Hash(name);
        assert(mDescriptorSets.count(hash) > 0);
        return mDescriptorSets.at(hash);
    }
    std::vector<uintptr_t> DescriptorService::DescriptorSetsBuffersAddrs(const std::string& name, 
        uint32_t idx, bool alredyAddedBaseAddress) const
    {
        //Get the descriptor set bucket that i want
        auto hash = utils::Hash(name);
        auto x = mDescriptorSetsBuffersOffsets.count(hash);
        assert(x > 0);
        auto allOffsets = mDescriptorSetsBuffersOffsets.at(hash);
        assert(idx * MAX_FRAMES_IN_FLIGHT < allOffsets.size());
     
        uintptr_t baseAddress = mBaseAddesses.at(hash);
        //now that only the descriptor set that i want
        std::vector<uintptr_t> slice(MAX_FRAMES_IN_FLIGHT);
        for (auto i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            if (!alredyAddedBaseAddress) {
                slice[i] = allOffsets[idx * MAX_FRAMES_IN_FLIGHT + i] + baseAddress;
            }
            else {
                slice[i] = allOffsets[idx * MAX_FRAMES_IN_FLIGHT + i];
            }
        }
        return slice;
    }
    uintptr_t DescriptorService::ModelMatrixDescriptorSetDynamicOffset( uint32_t idx)
    {
        const VkPhysicalDevice physicalDevice = Instance::gInstance->GetPhysicalDevice();
        auto hash = utils::Hash(MODEL_MATRIX_LAYOUT_NAME);
        assert(mDescriptorSetsBuffersOffsets.count(hash) > 0);
        constexpr size_t modelDataSize = sizeof(entities::ModelMatrixUniformBuffer);
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
        const VkDeviceSize minUniformBufferOffsetAlignment = deviceProperties.limits.minUniformBufferOffsetAlignment;
        const VkDeviceSize alignedModelDataSize = (modelDataSize + minUniformBufferOffsetAlignment - 1) & ~(minUniformBufferOffsetAlignment - 1);
        return idx * alignedModelDataSize;
    }
    DescriptorService::DescriptorSetBuffer DescriptorService::CreateBuffer(
    uint32_t numElements, VkDeviceSize sizeOfEachElement, 
        VkMemoryPropertyFlags memoryType, VkBufferUsageFlags usage, uintptr_t& baseAddress)
    {
        DescriptorService::DescriptorSetBuffer bufferData;
        /////1) create the big buffer
        VkBufferCreateInfo bigAssBufferInfo{};
        bigAssBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bigAssBufferInfo.size = sizeOfEachElement * numElements;
        bigAssBufferInfo.usage = usage;
        bigAssBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        VkBuffer bigBuffer = VK_NULL_HANDLE;
        vkCreateBuffer(Device::gDevice->GetDevice(), &bigAssBufferInfo, nullptr, 
            &bigBuffer);
        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(Device::gDevice->GetDevice(), bigBuffer, &memRequirements);
        VkMemoryAllocateInfo memoryAllocInfo{};
        memoryAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        memoryAllocInfo.allocationSize = memRequirements.size;
        memoryAllocInfo.memoryTypeIndex = utils::FindMemoryType(
            memRequirements.memoryTypeBits,
            memoryType);
        //Allocate the memory for the big buffer
        VkDeviceMemory bigBufferMemory;
        vkAllocateMemory(Device::gDevice->GetDevice(), &memoryAllocInfo, nullptr, 
            &bigBufferMemory);
        vkBindBufferMemory(Device::gDevice->GetDevice(), 
            bigBuffer, bigBufferMemory, 0);
        bufferData.mBuffer = bigBuffer;
        bufferData.mMemory = bigBufferMemory;
        void* baseAddr;
        vkMapMemory(Device::gDevice->GetDevice(), bigBufferMemory, 0,
            sizeOfEachElement * numElements, 0, &baseAddr);
        bufferData.mBasePointer = reinterpret_cast<std::uintptr_t>(baseAddr);
        baseAddress = reinterpret_cast<uintptr_t>(baseAddr);
        return bufferData;
    }
    VkDescriptorSetLayout DescriptorService::CreateDescriptorSetLayoutForCamera()
    {
        VkDevice device = vk::Device::gDevice->GetDevice();
        VkDescriptorSetLayoutBinding cameraLayoutBinding{};
        cameraLayoutBinding.binding = 0;
        cameraLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER ;
        cameraLayoutBinding.descriptorCount = 1;
        cameraLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        cameraLayoutBinding.pImmutableSamplers = nullptr; // Optional

        VkDescriptorSetLayoutCreateInfo cameraLayoutInfo{};
        cameraLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        cameraLayoutInfo.bindingCount = 1;
        cameraLayoutInfo.pBindings = &cameraLayoutBinding;
        VkDescriptorSetLayout layout;
        if (vkCreateDescriptorSetLayout(device, &cameraLayoutInfo,
            nullptr, &layout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor set layout!");
        }
        SET_NAME(layout, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, CAMERA_LAYOUT_NAME.c_str());
        return layout;
    }
    VkDescriptorPool DescriptorService::CreateDescriptorPoolForCamera()
    {
        //camera descriptor pool - there will be only one camera
        VkDescriptorPoolSize cameraPoolSize{};
        cameraPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        cameraPoolSize.descriptorCount = MAX_NUMBER_OF_CAMERAS;

        VkDescriptorPoolCreateInfo cameraPoolInfo{};
        cameraPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        cameraPoolInfo.poolSizeCount = 1;
        cameraPoolInfo.pPoolSizes = &cameraPoolSize;
        cameraPoolInfo.maxSets = MAX_NUMBER_OF_CAMERAS; // Only one camera. NEVER FORGET TO MULTIPLY BY MAX_FRAMES_IN_
        VkDescriptorPool descriptorPool;
        if (vkCreateDescriptorPool(Device::gDevice->GetDevice(), &cameraPoolInfo, 
            nullptr, &descriptorPool) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create camera descriptor pool!");
        }
        SET_NAME(descriptorPool, VK_OBJECT_TYPE_DESCRIPTOR_POOL, 
            CAMERA_LAYOUT_NAME.c_str());
        return descriptorPool;

    }
    const std::vector<VkDescriptorSet> DescriptorService::CreateDescriptorSetForCamera()
    {
        //alloc the buffers for the sets
        VkDeviceSize bufferSize = sizeof(entities::CameraUniformBuffer);
        uintptr_t baseAddress;
        DescriptorSetBuffer cameraBuffer = CreateBuffer(MAX_NUMBER_OF_CAMERAS, 
            sizeof(entities::CameraUniformBuffer),
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, baseAddress);
        mBaseAddesses.insert({ utils::Hash(CAMERA_LAYOUT_NAME), baseAddress });
        mDescriptorSetBuffers.insert({ utils::Hash(CAMERA_LAYOUT_NAME), cameraBuffer });
        auto bufferName = Concatenate(CAMERA_LAYOUT_NAME, "Buffer");
        //name the buffer objects
        SET_NAME(cameraBuffer.mBuffer, VK_OBJECT_TYPE_BUFFER, bufferName.c_str());
        auto memoryName = Concatenate(CAMERA_LAYOUT_NAME, "Memory");
        SET_NAME(cameraBuffer.mMemory, VK_OBJECT_TYPE_DEVICE_MEMORY, memoryName.c_str());
        //Build the descriptor sets
        VkDescriptorSetLayout cameraDescriptorSetLayout = mLayouts[utils::Hash(CAMERA_LAYOUT_NAME)];
        VkDescriptorPool cameraDescriptorPool = mDescriptorPools[utils::Hash(CAMERA_LAYOUT_NAME)];
        std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, cameraDescriptorSetLayout);
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = cameraDescriptorPool;
        allocInfo.descriptorSetCount = MAX_NUMBER_OF_CAMERAS;
        allocInfo.pSetLayouts = layouts.data();
        std::vector<VkDescriptorSet> descriptorSets;
        descriptorSets.resize(MAX_NUMBER_OF_CAMERAS);
        if (vkAllocateDescriptorSets(Device::gDevice->GetDevice(), &allocInfo, 
            descriptorSets.data()) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate descriptor sets for camera!");
        }
        std::vector<std::uintptr_t> descriptorSetOffsets;
        for (size_t i = 0; i < MAX_NUMBER_OF_CAMERAS; i++) {
            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = cameraBuffer.mBuffer;
            bufferInfo.offset = 0;
            bufferInfo.range = sizeof(entities::CameraUniformBuffer);
            VkWriteDescriptorSet descriptorWrite{};
            descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrite.dstSet = descriptorSets[i];
            descriptorWrite.dstBinding = 0;
            descriptorWrite.dstArrayElement = 0;
            descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrite.descriptorCount = 1;
            descriptorWrite.pBufferInfo = &bufferInfo;
            vkUpdateDescriptorSets(Device::gDevice->GetDevice(), 1, &descriptorWrite, 0, nullptr);
            descriptorSetOffsets.push_back(sizeof(entities::CameraUniformBuffer) * i);
        }
        //store their offsets
        mDescriptorSetsBuffersOffsets.insert({ utils::Hash(CAMERA_LAYOUT_NAME), descriptorSetOffsets });
        //name the descriptor sets
        int i = 0;
        for (auto& ds : descriptorSets) {
            auto n = Concatenate(CAMERA_LAYOUT_NAME.c_str(), "DescriptorSet", i);
            SET_NAME(ds, VK_OBJECT_TYPE_DESCRIPTOR_SET, n.c_str());
            i++;
        }
        return descriptorSets;
    }
    VkDescriptorSetLayout DescriptorService::CreateDescriptorSetLayoutForObject()
    {
        VkDevice device = Device::gDevice->GetDevice();
        VkDescriptorSetLayoutBinding modelMatrixBinding{};
        modelMatrixBinding.binding = 0;  // Will be the only resource in the set, so it's binding is 0
        modelMatrixBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;  // Dynamic uniform buffer type
        modelMatrixBinding.descriptorCount = 1;  // One descriptor for the model matrix buffer
        modelMatrixBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;  // Used in the vertex shader
        modelMatrixBinding.pImmutableSamplers = nullptr;  // Not using any immutable samplers
        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = 1;  // We have one binding (the dynamic uniform buffer)
        layoutInfo.pBindings = &modelMatrixBinding;  // Pointer to the binding array
        VkDescriptorSetLayout descriptorSetLayout;
        if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create descriptor set layout!");
        }
        SET_NAME(descriptorSetLayout, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT,
             MODEL_MATRIX_LAYOUT_NAME.c_str());
        return descriptorSetLayout;
    }
    VkDescriptorPool DescriptorService::CreateDescriptorPoolForModelMatrix()
    {
        VkDevice device = Device::gDevice->GetDevice();
        ///Create the descriptor pool.Because we are using dynamic buffers we create
        ///only one descriptor set per frame.
        std::array<VkDescriptorPoolSize, 1> poolSizes{};
        poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        poolSizes[0].descriptorCount = MAX_FRAMES_IN_FLIGHT;  // One descriptor for each frame in flight
        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes = poolSizes.data();
        poolInfo.maxSets = MAX_FRAMES_IN_FLIGHT;  // Number of descriptor sets = number of frames in flight
        VkDescriptorPool descriptorPool;
        if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create descriptor pool!");
        }
        SET_NAME(descriptorPool, VK_OBJECT_TYPE_DESCRIPTOR_POOL, "objectDescriptorPool");
        return descriptorPool;
    }
    const std::vector<VkDescriptorSet> DescriptorService::CreateDescriptorSetForModelMatrix()
    {
        const VkDevice device = Device::gDevice->GetDevice();
        const VkPhysicalDevice physicalDevice = Instance::gInstance->GetPhysicalDevice();
        const VkDescriptorSetLayout descriptorSetLayout = mLayouts.at(utils::Hash(MODEL_MATRIX_LAYOUT_NAME));
        const VkDescriptorPool descriptorPool = mDescriptorPools.at(utils::Hash(MODEL_MATRIX_LAYOUT_NAME));
        //Create the buffer
        //1)Calculate the buffer size. Since i'm using dynamic uniform buffers, alignment matters
        constexpr size_t modelDataSize = sizeof(entities::ModelMatrixUniformBuffer);
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
        const VkDeviceSize minUniformBufferOffsetAlignment = deviceProperties.limits.minUniformBufferOffsetAlignment;
        const VkDeviceSize alignedModelDataSize = (modelDataSize + minUniformBufferOffsetAlignment - 1) & ~(minUniformBufferOffsetAlignment - 1);
        const VkDeviceSize perFrameBufferSize = alignedModelDataSize * MAX_NUMBER_OF_OBJECTS;
        const VkDeviceSize totalBufferSize = perFrameBufferSize * MAX_FRAMES_IN_FLIGHT;
        //2)Create the buffer, it's memory and map to the base of the buffer.
        uintptr_t baseAddress;
        DescriptorSetBuffer modelBuffer = CreateBuffer(1,
            totalBufferSize,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, baseAddress);
        auto bufferName = Concatenate(MODEL_MATRIX_LAYOUT_NAME, "Buffer");
        auto memoryName = Concatenate(MODEL_MATRIX_LAYOUT_NAME, "Memory");
        SET_NAME(modelBuffer.mBuffer, VK_OBJECT_TYPE_BUFFER, bufferName.c_str());
        SET_NAME(modelBuffer.mMemory, VK_OBJECT_TYPE_DEVICE_MEMORY, memoryName.c_str());
        //3)Create the descriptor sets
        std::vector<VkDescriptorSet> descriptorSets(MAX_FRAMES_IN_FLIGHT);
        std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, 
            descriptorSetLayout);
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = descriptorPool;  // The pool created earlier
        allocInfo.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
        allocInfo.pSetLayouts = layouts.data();
        if (vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
            throw std::runtime_error("Failed to allocate descriptor sets!");
        }
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = modelBuffer.mBuffer;  // The buffer you created
            bufferInfo.offset = 0;              // Dynamic offset will control the starting point
            bufferInfo.range = alignedModelDataSize;  // This should be the aligned size of one ModelData

            VkWriteDescriptorSet descriptorWrite{};
            descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrite.dstSet = descriptorSets[i];   // Descriptor set for the current frame
            descriptorWrite.dstBinding = 0;               // Binding point in the shader
            descriptorWrite.dstArrayElement = 0;
            descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
            descriptorWrite.descriptorCount = 1;
            descriptorWrite.pBufferInfo = &bufferInfo;

            vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);
        }
        //4)pre calculate the dynamic offsets
        std::vector<std::uintptr_t> dynamicOffsets(MAX_NUMBER_OF_OBJECTS);
        for (uint32_t i = 0; i < MAX_NUMBER_OF_OBJECTS; i++) {
            dynamicOffsets[i] = i * alignedModelDataSize;
        }
        //5)fill the tables
        mDescriptorSetBuffers.insert({ utils::Hash(MODEL_MATRIX_LAYOUT_NAME), modelBuffer});
        mBaseAddesses.insert({ utils::Hash(MODEL_MATRIX_LAYOUT_NAME), baseAddress });
        mDescriptorSetsBuffersOffsets.insert({ utils::Hash(MODEL_MATRIX_LAYOUT_NAME), dynamicOffsets });
        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            auto name = Concatenate(MODEL_MATRIX_LAYOUT_NAME, "DescriptorSetForFrame", i);
            SET_NAME(descriptorSets[i], VK_OBJECT_TYPE_DESCRIPTOR_SET, name.c_str());
        }
        return descriptorSets;
    }
    VkDescriptorSetLayout DescriptorService::CreateDescriptorSetLayoutForSampler()
    {
        VkDevice device = Device::gDevice->GetDevice();
        VkDescriptorSetLayoutBinding samplerLayoutBinding{};
        samplerLayoutBinding.binding = 0;
        samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        samplerLayoutBinding.descriptorCount = 1;
        samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        samplerLayoutBinding.pImmutableSamplers = nullptr; // Optional
        VkDescriptorSetLayoutCreateInfo samplerLayoutInfo{};
        samplerLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        samplerLayoutInfo.bindingCount = 1;
        samplerLayoutInfo.pBindings = &samplerLayoutBinding;
        VkDescriptorSetLayout layout;
        if (vkCreateDescriptorSetLayout(device, &samplerLayoutInfo,
            nullptr, &layout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor set layout for sampler");
        }
        SET_NAME(layout, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT,
            SAMPLER_LAYOUT_NAME.c_str());
        return layout;
    }
    VkDescriptorPool DescriptorService::CreateDescriptorPoolForSampler()
    {
        //////CREATES THE DESCRIPTOR POOL FOR SAMPLERS//////
        const int numberOfSamplers = SAMPLER_POOL_SIZE * MAX_FRAMES_IN_FLIGHT;
        VkDescriptorPoolSize samplerPoolSize{};
        samplerPoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        samplerPoolSize.descriptorCount = numberOfSamplers;

        VkDescriptorPoolCreateInfo samplerPoolInfo{};
        samplerPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        samplerPoolInfo.poolSizeCount = 1;
        samplerPoolInfo.pPoolSizes = &samplerPoolSize;
        samplerPoolInfo.maxSets = numberOfSamplers;
        VkDescriptorPool descriptorPool;
        if (vkCreateDescriptorPool(Device::gDevice->GetDevice(), &samplerPoolInfo, 
            nullptr, &descriptorPool) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create sampler descriptor pool!");
        }
        SET_NAME(descriptorPool, VK_OBJECT_TYPE_DESCRIPTOR_POOL, 
            SAMPLER_LAYOUT_NAME.c_str());
        return descriptorPool;

    }
    std::vector<VkDescriptorSet> DescriptorService::GenerateSamplerDescriptorSetsForTexture(VkDescriptorPool samplerDescriptorPool, VkDescriptorSetLayout samplerDescriptorSetLayout, VkImageView imageView, VkSampler sampler, const std::string& name)
    {
        //one layout for each frame in flight, create the vector filling with the layout that i alredy have
        std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT,
            samplerDescriptorSetLayout);
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = samplerDescriptorPool;
        //one descriptor set for each frame in flight
        allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        allocInfo.pSetLayouts = layouts.data();
        std::vector<VkDescriptorSet> descriptorSets(MAX_FRAMES_IN_FLIGHT);
        if (vkAllocateDescriptorSets(vk::Device::gDevice->GetDevice(), &allocInfo,
            descriptorSets.data()) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate descriptor sets");
        }
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            VkDescriptorImageInfo imageInfo{};
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo.imageView = imageView;
            imageInfo.sampler = sampler;
            VkWriteDescriptorSet descriptorWrite{};
            descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrite.dstSet = descriptorSets[i];
            descriptorWrite.dstBinding = 0;
            descriptorWrite.dstArrayElement = 0;
            descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptorWrite.descriptorCount = 1;
            descriptorWrite.pImageInfo = &imageInfo;
            vkUpdateDescriptorSets(Device::gDevice->GetDevice(), 1, &descriptorWrite, 0, nullptr);
        }
        return descriptorSets;
    }
    void DescriptorService::CreateDescriptorSetLayoutForLight()
    {
        //directional light
        VkDescriptorSetLayoutBinding directionalLightBinding = {};
        directionalLightBinding.binding = 0; // Corresponds to set = 2, binding = 0
        directionalLightBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        directionalLightBinding.descriptorCount = 1;
        directionalLightBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT;
        directionalLightBinding.pImmutableSamplers = nullptr; // Not used for uniform buffers
        //ambient light
        VkDescriptorSetLayoutBinding ambientLightBinding = {};
        ambientLightBinding.binding = 1; // Corresponds to set = 2, binding = 1
        ambientLightBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        ambientLightBinding.descriptorCount = 1;
        ambientLightBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        ambientLightBinding.pImmutableSamplers = nullptr;
        std::array<VkDescriptorSetLayoutBinding, 2> bindings = { directionalLightBinding, ambientLightBinding };
        VkDescriptorSetLayoutCreateInfo layoutInfo = {};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        layoutInfo.pBindings = bindings.data();
        VkDescriptorSetLayout descriptorSetLayout;
        if (vkCreateDescriptorSetLayout(vk::Device::gDevice->GetDevice(), &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor set layout!");
        }
        auto name = Concatenate(LIGHTNING_LAYOUT_NAME,"Layout");
        SET_NAME(descriptorSetLayout, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, name.c_str());
        mLayouts.insert({ utils::Hash(LIGHTNING_LAYOUT_NAME), descriptorSetLayout });
    }

    void DescriptorService::CreateDescriptorPoolForLight()
    {
        std::array<VkDescriptorPoolSize, 2> poolSizes;
        // Pool size for directional light (binding = 0, type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
        poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSizes[0].descriptorCount = MAX_FRAMES_IN_FLIGHT; // One for each frame in flight

        // Pool size for ambient light (binding = 1, type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
        poolSizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSizes[1].descriptorCount = MAX_FRAMES_IN_FLIGHT; // One for each frame in flight

        VkDescriptorPoolCreateInfo poolInfo = {};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = poolSizes.size(); // Two types of descriptors (directional and ambient light)
        poolInfo.pPoolSizes = poolSizes.data();
        poolInfo.maxSets = MAX_FRAMES_IN_FLIGHT; // One set per frame
        VkDescriptorPool descriptorPool;
        if (vkCreateDescriptorPool(vk::Device::gDevice->GetDevice(), &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor pool!");
        }
        auto name = Concatenate(LIGHTNING_LAYOUT_NAME, "Pool");
        SET_NAME(descriptorPool, VK_OBJECT_TYPE_DESCRIPTOR_POOL, name.c_str());
        mDescriptorPools.insert({ utils::Hash(LIGHTNING_LAYOUT_NAME), descriptorPool });
    }
    void DescriptorService::CreateDescriptorSetForLight()
    {
        const VkDevice device = Device::gDevice->GetDevice();
        const VkPhysicalDevice physicalDevice = Instance::gInstance->GetPhysicalDevice();
        const VkDescriptorSetLayout descriptorSetLayout = mLayouts.at(utils::Hash(LIGHTNING_LAYOUT_NAME));
        const VkDescriptorPool descriptorPool = mDescriptorPools.at(utils::Hash(LIGHTNING_LAYOUT_NAME));
        //1) Calculate the buffers sizes
        const VkDeviceSize directionalLightSize = sizeof(entities::DirectionalLightUniformBuffer) * MAX_FRAMES_IN_FLIGHT;
        const VkDeviceSize ambientLightSize = sizeof(entities::AmbientLightUniformBuffer) * MAX_FRAMES_IN_FLIGHT;
        //2) Create the buffers, their memories and maps
        uintptr_t directionalLightBaseAddress;
        DescriptorSetBuffer directionalLightBuffer = CreateBuffer(1,
            directionalLightSize,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, directionalLightBaseAddress);
        auto directionaLightbufferName = Concatenate(LIGHTNING_LAYOUT_NAME, "Buffer");
        auto directionaLightmemoryName = Concatenate(LIGHTNING_LAYOUT_NAME, "Memory");
        SET_NAME(directionalLightBuffer.mBuffer, VK_OBJECT_TYPE_BUFFER, directionaLightbufferName.c_str());
        SET_NAME(directionalLightBuffer.mMemory, VK_OBJECT_TYPE_DEVICE_MEMORY, directionaLightmemoryName.c_str());
        uintptr_t ambientLightBaseAddress;
        DescriptorSetBuffer ambientLightBuffer = CreateBuffer(1,
            directionalLightSize,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, ambientLightBaseAddress);
        auto ambientLightbufferName = Concatenate(LIGHTNING_LAYOUT_NAME, "Buffer");
        auto ambientLightmemoryName = Concatenate(LIGHTNING_LAYOUT_NAME, "Memory");
        SET_NAME(ambientLightBuffer.mBuffer, VK_OBJECT_TYPE_BUFFER, ambientLightbufferName.c_str());
        SET_NAME(ambientLightBuffer.mMemory, VK_OBJECT_TYPE_DEVICE_MEMORY, ambientLightmemoryName.c_str());
        //3)Create the descriptor sets
        std::vector<VkDescriptorSet> descriptorSets(MAX_FRAMES_IN_FLIGHT);
        std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);
        VkDescriptorSetAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = descriptorPool; // The descriptor pool you created
        allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        allocInfo.pSetLayouts = layouts.data();
        if (vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate descriptor sets!");
        }
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            VkDescriptorBufferInfo directionalLightBufferInfo = {};
            directionalLightBufferInfo.buffer = directionalLightBuffer.mBuffer;  // The buffer you created
            directionalLightBufferInfo.offset = 0;
            directionalLightBufferInfo.range = directionalLightSize;  // Total size of 16 lights (direction + color)

            VkDescriptorBufferInfo ambientLightBufferInfo = {};
            ambientLightBufferInfo.buffer = ambientLightBuffer.mBuffer;  // The buffer you created
            ambientLightBufferInfo.offset = 0;
            ambientLightBufferInfo.range = ambientLightSize;  // Size of the ambient light (vec4)

            std::array<VkWriteDescriptorSet, 2> descriptorWrites = {};

            // Binding 0: DirectionalLightUniformBuffer
            descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[0].dstSet = descriptorSets[i];
            descriptorWrites[0].dstBinding = 0;
            descriptorWrites[0].dstArrayElement = 0;
            descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrites[0].descriptorCount = 1;
            descriptorWrites[0].pBufferInfo = &directionalLightBufferInfo;

            // Binding 1: AmbientLightUniformBuffer
            descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[1].dstSet = descriptorSets[i];
            descriptorWrites[1].dstBinding = 1;
            descriptorWrites[1].dstArrayElement = 0;
            descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrites[1].descriptorCount = 1;
            descriptorWrites[1].pBufferInfo = &ambientLightBufferInfo;

            vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
        }
        //4) Calculate the offsets
        std::vector<uintptr_t> ambientAddrs;
        std::vector<uintptr_t> directionalAddrs;
        for (auto i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            uintptr_t currAmbientOffset = ambientLightSize * i;
            ambientAddrs.push_back(currAmbientOffset);
            uintptr_t currDirectionalOffset = directionalLightSize * i;
            directionalAddrs.push_back(currDirectionalOffset);
        }
        //5) save to the tables
        auto ambient_name = Concatenate(LIGHTNING_LAYOUT_NAME, "Ambient");
        mDescriptorSetBuffers.insert({ utils::Hash(ambient_name), ambientLightBuffer });
        mBaseAddesses.insert({ utils::Hash(ambient_name), ambientLightBaseAddress });
        mDescriptorSetsBuffersOffsets.insert({ utils::Hash(ambient_name), ambientAddrs });
        auto directional_name = Concatenate(LIGHTNING_LAYOUT_NAME, "Directional");
        mDescriptorSetBuffers.insert({ utils::Hash(directional_name), directionalLightBuffer });
        mBaseAddesses.insert({ utils::Hash(directional_name), directionalLightBaseAddress });
        mDescriptorSetsBuffersOffsets.insert({ utils::Hash(directional_name), directionalAddrs });
        mDescriptorSets.insert({ utils::Hash(LIGHTNING_LAYOUT_NAME), descriptorSets });
    }
    void DescriptorService::CreateDescriptorSetLayoutForFakeShadowBuffer()
    {
        const VkDevice device = Device::gDevice->GetDevice();
        // Step 1: Create Descriptor Set Layout
        VkDescriptorSetLayoutBinding colorSamplerLayoutBinding = {};
        colorSamplerLayoutBinding.binding = 0;
        colorSamplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        colorSamplerLayoutBinding.descriptorCount = 1;
        colorSamplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        colorSamplerLayoutBinding.pImmutableSamplers = nullptr;

        VkDescriptorSetLayoutBinding depthSamplerLayoutBinding = {};
        depthSamplerLayoutBinding.binding = 1;
        depthSamplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        depthSamplerLayoutBinding.descriptorCount = 1;
        depthSamplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        depthSamplerLayoutBinding.pImmutableSamplers = nullptr;

        std::array<VkDescriptorSetLayoutBinding, 2> bindings = { colorSamplerLayoutBinding, depthSamplerLayoutBinding };
        VkDescriptorSetLayoutCreateInfo layoutInfo = {};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        layoutInfo.pBindings = bindings.data();

        VkDescriptorSetLayout descriptorSetLayout;
        if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor set layout!");
        }
        mLayouts.insert({ utils::Hash(FAKE_SHADOW_MAP_SAMPLERS), descriptorSetLayout });
        auto n = Concatenate(FAKE_SHADOW_MAP_SAMPLERS, "_DescriptorSetLayouts");
        SET_NAME(descriptorSetLayout, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, n.c_str());
    }
    void DescriptorService::CreateDescriptorPoolForFakeShadowBuffer()
    {
        const VkDevice device = Device::gDevice->GetDevice();
        // Step 2: Create Descriptor Pool
        VkDescriptorPoolSize poolSize = {};
        poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSize.descriptorCount = 2;

        VkDescriptorPoolCreateInfo poolInfo = {};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = 1;
        poolInfo.pPoolSizes = &poolSize;
        poolInfo.maxSets = 1;

        VkDescriptorPool descriptorPool;
        if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor pool!");
        }
        auto n = Concatenate(FAKE_SHADOW_MAP_SAMPLERS, "_DescriptorPool");
        SET_NAME(descriptorPool, VK_OBJECT_TYPE_DESCRIPTOR_POOL, n.c_str());
        mDescriptorPools.insert({ FAKE_SHADOW_MAP_SAMPLERS, descriptorPool });
    }
    void DescriptorService::CreateDescriptorSetForFakeShadowBuffer(std::vector<VkImageView> colorImageViews, VkImageView depthImageView)
    {
        VkDescriptorSetLayout descriptorSetLayout = mLayouts.at(utils::Hash(FAKE_SHADOW_MAP_SAMPLERS));
        VkDescriptorPool descriptorPool = mDescriptorPools.at(utils::Hash(FAKE_SHADOW_MAP_SAMPLERS));
        VkDevice device = vk::Device::gDevice->GetDevice();
        
        std::vector<VkDescriptorSet> descriptorSet(MAX_FRAMES_IN_FLIGHT);
        std::array<VkDescriptorSetLayout, MAX_FRAMES_IN_FLIGHT> layouts;
        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) layouts[i] = descriptorSetLayout;
        VkDescriptorSetAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = descriptorPool;
        allocInfo.descriptorSetCount = descriptorSet.size();
        allocInfo.pSetLayouts = layouts.data();

        if (vkAllocateDescriptorSets(device, &allocInfo, descriptorSet.data()) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate descriptor set!");
        }
        VkSampler sampler = mSamplerService.GetSampler(LINEAR_REPEAT_NORMALIZED_NONMIPMAP_SAMPLER);
        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            const auto ds_name = Concatenate(FAKE_SHADOW_MAP_SAMPLERS, "DescriptorSet");
            SET_NAME(descriptorSet[i], VK_OBJECT_TYPE_DESCRIPTOR_SET, ds_name.c_str());
            VkDescriptorImageInfo colorImageInfo = {};
            colorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            colorImageInfo.imageView = colorImageViews[i]; // Image view from fakeShadowMapRenderPass
            colorImageInfo.sampler = sampler; 

            VkDescriptorImageInfo depthImageInfo = {};
            depthImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            depthImageInfo.imageView = depthImageView; // Image view from fakeShadowMapRenderPass
            depthImageInfo.sampler = sampler; 
            std::array<VkWriteDescriptorSet, 2> descriptorWrites = {};

            descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[0].dstSet = descriptorSet[i];
            descriptorWrites[0].dstBinding = 0;
            descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptorWrites[0].descriptorCount = 1;
            descriptorWrites[0].pImageInfo = &colorImageInfo;

            descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[1].dstSet = descriptorSet[i];
            descriptorWrites[1].dstBinding = 1;
            descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptorWrites[1].descriptorCount = 1;
            descriptorWrites[1].pImageInfo = &depthImageInfo;

            vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);

        }
        mDescriptorSets.insert({ utils::Hash(FAKE_SHADOW_MAP_SAMPLERS), descriptorSet });
    }
}