#pragma once
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <vulkan/vulkan.h>
#include <glm/gtc/quaternion.hpp>
#define MAX_NUMBER_OF_GAME_OBJECTS 10000
namespace vk {
    class DescriptorService;
}
namespace entities {

    class Mesh;
    class Pipeline;
    class GameObject {
    public:
        ~GameObject();
        GameObject(const std::string& name, 
            vk::DescriptorService& descriptorService,
            const std::string& pipeline,  //TODO staticMeshRenderer: Game Object should not have the mesh
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
        const size_t mPipelineHash;
        const std::string mPipelineName;
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