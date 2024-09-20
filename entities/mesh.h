#pragma once
#include <string>
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <vector>
#include <array>
#include "entities/mesh-data.h"

namespace entities {
    //The vertex data structure, for a 3d vertex and it's color
    struct Vertex {
        glm::vec3 pos;
        glm::vec2 uv0;
        glm::vec3 normal;
    };
    class Mesh {
    public:
        Mesh(entities::MeshData& meshData);
        ~Mesh();
        const std::string mName;
        void Bind(VkCommandBuffer cmd)const;
        uint16_t NumberOfIndices()const {
            return mNumberOfIndices;
        }
    private:
        void CtorStartAssertions();
        void CtorInitGlobalMeshBuffer();
        void CtorCopyDataToGlobalBuffer(
            const std::vector<Vertex>& vertexes,
            const std::vector<uint16_t>& indices);
        VkDeviceSize mVertexesOffset;
        VkDeviceSize mIndexesOffset;
        uint16_t mNumberOfIndices;
        
    };
}