#include "mesh.h"
#include <cassert>
#include <vk\device.h>
#include <vk\instance.h>
#include <vk\debug_utils.h>
#include <utils\concatenate.h>
#include <utils\vk_utils.h>

#define _256mb 256 * 1024 * 1024
static VkBuffer gMeshBuffer = VK_NULL_HANDLE;
static VkDeviceMemory gMeshMemory = VK_NULL_HANDLE;
uint32_t meshCounter = 0;
uintptr_t gMemoryCursor = 0;

namespace entities {
    std::vector<VkVertexInputAttributeDescription> Mesh::AttributeDescription()
    {
        std::vector<VkVertexInputAttributeDescription> attributeDescriptions(3);
        //inPosition
        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(entities::Vertex, pos);
        //inUV0
        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(entities::Vertex, uv0);
        //inColor
        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[2].offset = offsetof(entities::Vertex, normal);
        return attributeDescriptions;
    }
    VkVertexInputBindingDescription Mesh::BindingDescription()
    {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(entities::Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return bindingDescription;
    }
    Mesh::Mesh(entities::MeshData& meshData):mName(meshData.name)
    {
        CtorStartAssertions();
        //If this is the first mesh then we have to create the infrastructure.
        CtorInitGlobalMeshBuffer();
        assert(gMeshBuffer != nullptr);
        assert(gMeshMemory != nullptr);
        meshCounter++;
        std::vector<entities::Vertex> vertices(meshData.vertices.size());
        for (auto i = 0; i < meshData.vertices.size(); i++) {
            vertices[i].pos = meshData.vertices[i];
            vertices[i].uv0 = meshData.uv0s[i];
            vertices[i].normal = meshData.normals[i];
        }
        std::vector<uint16_t> indices = meshData.indices;
        CtorCopyDataToGlobalBuffer(vertices, indices);

    }
    Mesh::~Mesh()
    {
        assert(
            vk::Device::gDevice->GetDevice() != VK_NULL_HANDLE && 
            gMeshBuffer != VK_NULL_HANDLE && 
            gMeshMemory != VK_NULL_HANDLE);
        meshCounter--;
        if (meshCounter == 0) {
            vkFreeMemory(vk::Device::gDevice->GetDevice(), gMeshMemory, nullptr);
            vkDestroyBuffer(vk::Device::gDevice->GetDevice(), gMeshBuffer, nullptr);
        }
    }
    void Mesh::Bind(VkCommandBuffer cmd) const
    {
        assert(mIndexesOffset != LLONG_MAX);
        assert(mVertexesOffset != LLONG_MAX);
        vkCmdBindVertexBuffers(cmd, 0, 1, &gMeshBuffer, &mVertexesOffset);
        vkCmdBindIndexBuffer(cmd, gMeshBuffer, mIndexesOffset, VK_INDEX_TYPE_UINT16);
    }
    void Mesh::CtorStartAssertions()
    {
        assert(vk::Device::gDevice->GetDevice() != VK_NULL_HANDLE);
        assert(vk::Device::gDevice->GetCommandPool() != VK_NULL_HANDLE);
        assert(vk::Device::gDevice->GetGraphicsQueue() != VK_NULL_HANDLE);
    }
    void Mesh::CtorInitGlobalMeshBuffer()
    {
        //If this is the first mesh then we have to create the infrastructure.
        if (gMeshBuffer == VK_NULL_HANDLE && gMeshMemory == VK_NULL_HANDLE) {
            assert(meshCounter == 0);
            VkDeviceSize vbSize = _256mb; //256 mb for meshes
            //Buffer description
            VkBufferCreateInfo vbBufferInfo{};
            vbBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            vbBufferInfo.size = vbSize;
            vbBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
            vbBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            //create the buffer
            if (vkCreateBuffer(vk::Device::gDevice->GetDevice(), &vbBufferInfo, nullptr, &gMeshBuffer) != VK_SUCCESS) {
                throw std::runtime_error("failed to create buffer!");
            }
            //the memory the buffer will require, not necessarely equals to the size of the data being stored
            //in it.
            VkMemoryRequirements memRequirements;
            vkGetBufferMemoryRequirements(vk::Device::gDevice->GetDevice(), gMeshBuffer, &memRequirements);
            VkMemoryAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocInfo.allocationSize = memRequirements.size;
            allocInfo.memoryTypeIndex = utils::FindMemoryType(memRequirements.memoryTypeBits,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
            //Allocate the memory
            if (vkAllocateMemory(vk::Device::gDevice->GetDevice(), &allocInfo, nullptr, &gMeshMemory) != VK_SUCCESS) {
                throw std::runtime_error("failed to allocate buffer memory!");
            }
            vkBindBufferMemory(vk::Device::gDevice->GetDevice(), gMeshBuffer, gMeshMemory, 0);
            SET_NAME(gMeshBuffer, VK_OBJECT_TYPE_BUFFER, "Global Mesh Buffer");
            SET_NAME(gMeshMemory, VK_OBJECT_TYPE_DEVICE_MEMORY, "Global Mesh Memory");
        }
    }
    void Mesh::CtorCopyDataToGlobalBuffer(const std::vector<Vertex>& vertexes, const std::vector<uint16_t>& indices)
    {
        //copy from the vectors to the gpu using staging buffers. Mind the offsets.
        //1)Create a local buffer for the vertex buffer
        VkDeviceSize vertexBufferSize = sizeof(vertexes[0]) * vertexes.size();
        //creates the staging buffer
        VkBuffer vertexStagingBuffer;
        VkDeviceMemory vertexStagingBufferMemory;
        utils::CreateBuffer(vertexBufferSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT, //Used as source from memory transfers
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, //memory is visibe to the cpu and gpu
            vertexStagingBuffer, vertexStagingBufferMemory);
        void* vertexStagingBufferAddress;
        vkMapMemory(vk::Device::gDevice->GetDevice(), vertexStagingBufferMemory, 0, vertexBufferSize, 0, &vertexStagingBufferAddress);
        memcpy(vertexStagingBufferAddress, vertexes.data(), (size_t)vertexBufferSize);
        vkUnmapMemory(vk::Device::gDevice->GetDevice(), vertexStagingBufferMemory);
        //2)Create a local buffer for the index buffer
        VkDeviceSize indexBufferSize = sizeof(indices[0]) * indices.size();
        VkBuffer indexStagingBuffer;
        VkDeviceMemory indexStagingBufferMemory;
        utils::CreateBuffer(indexBufferSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT, //Used as source from memory transfers
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, //memory is visibe to the cpu and gpu
            indexStagingBuffer, indexStagingBufferMemory);
        void* indexStagingBufferAddress;
        vkMapMemory(vk::Device::gDevice->GetDevice(), indexStagingBufferMemory, 0, indexBufferSize, 0, &indexStagingBufferAddress);
        memcpy(indexStagingBufferAddress, indices.data(), (size_t)indexBufferSize);
        vkUnmapMemory(vk::Device::gDevice->GetDevice(), indexStagingBufferMemory);
        assert(gMemoryCursor + vertexBufferSize + indexBufferSize < _256mb); //Is there enough space?
        //3)Copy the vertex and index buffer to the main buffer, increase the cursor
        VkCommandBuffer vbCopyCommandBuffer = vk::Device::gDevice->CreateCommandBuffer("vertex buffer copy command buffer");
        vk::Device::gDevice->BeginRecordingCommands(vbCopyCommandBuffer);
        vk::Device::gDevice->CopyBuffer(0, gMemoryCursor, vertexBufferSize, vbCopyCommandBuffer, vertexStagingBuffer, gMeshBuffer);
        mVertexesOffset = gMemoryCursor;
        gMemoryCursor += vertexBufferSize;
        vk::Device::gDevice->CopyBuffer(0, gMemoryCursor, indexBufferSize, vbCopyCommandBuffer, indexStagingBuffer, gMeshBuffer);
        mIndexesOffset = gMemoryCursor;
        gMemoryCursor += indexBufferSize;
        //4)Finish execution
        vk::Device::gDevice->SubmitAndFinishCommands(vbCopyCommandBuffer);
        mNumberOfIndices = static_cast<uint16_t>(indices.size());

        vkDestroyBuffer(vk::Device::gDevice->GetDevice(), vertexStagingBuffer, nullptr);
        vkDestroyBuffer(vk::Device::gDevice->GetDevice(), indexStagingBuffer, nullptr);
        vkFreeMemory(vk::Device::gDevice->GetDevice(), vertexStagingBufferMemory, nullptr);
        vkFreeMemory(vk::Device::gDevice->GetDevice(), indexStagingBufferMemory, nullptr);
    }
}
