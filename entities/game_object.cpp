#include "game_object.h"
#include <map>
#include "mesh.h"
#include <vk\descriptor_service.h>
#include <utils/hash.h>
#include "pipeline.h"
#include <vk/debug_utils.h>
#include "model_matrix_uniform_buffer.h"
#include "camera_uniform_buffer.h"
/// <summary>
/// List of available id for game objects. Its important because there's a limited number of possible
/// game objects, limited by the number of descriptor sets in the pool and memory allocated on the 
/// uniform buffer. The game object id is directly related to the offset in the memory and the specific
/// descriptor set bound to that offset. 
/// </summary>
static std::map<uint32_t, bool> gAvailableGameObjectsIds;
/// <summary>
/// Returns an id in [0, MAX_NUMBER_OF_GAME_OBJECTS). If all IDs are used it blows up with runtime error
/// </summary>
/// <returns></returns>
uint32_t GetNextGameObjectId() {
    //initialize the list
    if (gAvailableGameObjectsIds.size() == 0) {
        for (uint32_t i = 0; i < MAX_NUMBER_OF_GAME_OBJECTS; i++) {
            gAvailableGameObjectsIds.insert({ i, true });
        }
    }
    int key = UINT32_MAX;
    for (const auto& [k, v] : gAvailableGameObjectsIds) {
        if (v == true) {
            key = k;
            break;
        }
    }
    if (key == UINT32_MAX) {
        throw std::runtime_error("all possible game objects are alredy allocated");
    }
    return key;
}
namespace entities
{
    GameObject::GameObject(const std::string& name, 
        vk::DescriptorService& descriptorService, 
        const std::string& pipeline,
        entities::Mesh* mesh)
        :mName(name), mId(GetNextGameObjectId()), mDescriptorService(descriptorService), mMesh(mesh),
        mPipelineName(pipeline), mPipelineHash(utils::Hash(pipeline))
    {
        //Get the descriptor sets
        mModelMatrixDescriptorSet = mDescriptorService.DescriptorSet(vk::OBJECT_LAYOUT_NAME, mId);
        mModelMatrixOffset = mDescriptorService.DescriptorSetsBuffersAddrs(vk::OBJECT_LAYOUT_NAME, mId);
        mCameraDescriptorSet = mDescriptorService.DescriptorSet(vk::CAMERA_LAYOUT_NAME, mId);;
        mCameraOffset = mDescriptorService.DescriptorSetsBuffersAddrs(vk::CAMERA_LAYOUT_NAME, mId);
    }

    void GameObject::Draw(VkCommandBuffer cmdBuffer, 
        Pipeline& pipeline, uint32_t currentFrame)
    {
        vk::SetMark({ 1.0f, 0.8f, 1.0f, 1.0f }, mName, cmdBuffer);
        CopyModelMatrixToDescriptorSetMemory(currentFrame);
        //bind the mesh.
        mMesh->Bind(cmdBuffer);
        //bind the descriptor sets
        uint32_t cameraDynamicOffset = DynamicOffset<entities::CameraUniformBuffer>(currentFrame, 0);
        vkCmdBindDescriptorSets(
            cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
            pipeline.PipelineLayout(),
            vk::CAMERA_SET,
            1,
            &mCameraDescriptorSet[currentFrame],
            1,
            &cameraDynamicOffset
        );
        uint32_t modelMatrixDynamicOffset = DynamicOffset<entities::ModelMatrixUniformBuffer>(currentFrame, mId);
        vkCmdBindDescriptorSets(
            cmdBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            pipeline.PipelineLayout(),
            vk::MODEL_MATRIX_SET,                                  
            1,                                 
            &mModelMatrixDescriptorSet[currentFrame],               
            1,
            &modelMatrixDynamicOffset
        );
        //Draw command
        vkCmdDrawIndexed(cmdBuffer,
            static_cast<uint32_t>(mMesh->NumberOfIndices()),
            1,
            0,
            0,
            0);
        vk::EndMark(cmdBuffer);
    }

    void GameObject::CopyModelMatrixToDescriptorSetMemory(uint32_t currentFrame)
    {
        //send the model matrix to the gpu
        entities::ModelMatrixUniformBuffer modelMatrixUniformBuffer;
        modelMatrixUniformBuffer.model = glm::mat4(1.0f);
        modelMatrixUniformBuffer.model *= glm::translate(glm::mat4(1.0f), mPosition);
        modelMatrixUniformBuffer.model *= glm::mat4_cast(mOrientation);
        void* modelMatUB = reinterpret_cast<void*>(mModelMatrixOffset[currentFrame]);
        memcpy(modelMatUB, &modelMatrixUniformBuffer, sizeof(entities::ModelMatrixUniformBuffer));
    }
}
