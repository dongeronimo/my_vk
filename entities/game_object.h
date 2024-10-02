#pragma once
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <functional>
#include <optional>
#include <vulkan/vulkan.h>
#include <glm/gtc/quaternion.hpp>
#include "utils/hash.h"
#define MAX_NUMBER_OF_GAME_OBJECTS 10000
namespace vk {
    class DescriptorService;
}
namespace entities {

    class Mesh;
    class Pipeline;
    class GameObject {
    public:
        std::optional<std::function<void(GameObject&, Pipeline&, uint32_t, VkCommandBuffer)>> OnDraw;
        ~GameObject();
        GameObject(const std::string& name, 
            vk::DescriptorService& descriptorService,
            entities::Mesh* mesh);//TODO staticMeshRenderer: Game Object should not have the mesh
        void SetPosition(glm::vec3& pos) {
            mPosition = pos;
        }
        glm::vec3 GetPosition()const { return mPosition; }
        void SetOrientation(glm::quat& o) {
            mOrientation = o;
        }
        glm::quat GetOrientation()const { return mOrientation; }
        const std::string mName;
        const uint32_t mId;
        void Draw(VkCommandBuffer cmdBuffer, 
            Pipeline& pipeline,
            uint32_t currentFrame);//TODO staticMeshRenderer: this belongs to StaticMeshRenderer
    private:
        void CopyModelMatrixToDescriptorSetMemory(uint32_t currentFrame);//TODO staticRenderMesh: 
        entities::Mesh* mMesh;
        vk::DescriptorService& mDescriptorService;
        std::vector<VkDescriptorSet> mModelMatrixDescriptorSet;
        //std::vector<uintptr_t> mModelMatrixOffset;
        std::vector<VkDescriptorSet> mCameraDescriptorSet;
        std::vector<uintptr_t> mCameraOffset;
        glm::vec3 mPosition;
        glm::quat mOrientation;
    };
}