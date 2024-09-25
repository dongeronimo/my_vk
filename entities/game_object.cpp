#include "game_object.h"
#include <map>
#include <vk\descriptor_service.h>
#include <utils/hash.h>
#include <vk/debug_utils.h>
#include "model_matrix_uniform_buffer.h"
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
    GameObject::GameObject(const std::string& name, vk::DescriptorService& descriptorService, 
        const std::string& pipeline,
        entities::Mesh* mesh)
        :mName(name), mId(GetNextGameObjectId()), mDescriptorService(descriptorService), mMesh(mesh),
        mPipelineName(pipeline), mPipelineHash(utils::Hash(pipeline))
    {
        //Get the descriptor set for model matrix
        mDescriptorSets = mDescriptorService.DescriptorSet(vk::OBJECT_LAYOUT_NAME, mId);
        mOffsets = mDescriptorService.DescriptorSetsBuffersAddrs(vk::OBJECT_LAYOUT_NAME, mId);
    }
    void GameObject::Draw(VkCommandBuffer cmdBuffer, uint32_t currentFrame)
    {
        vk::SetMark({ 1.0f, 0.8f, 1.0f, 1.0f }, mName, cmdBuffer);
        CopyModelMatrixToDescriptorSetMemory(currentFrame);
        //TODO render: bind the mesh
        //TODO render: bind the descriptor sets
        vk::EndMark(cmdBuffer);
    }
    void GameObject::CopyModelMatrixToDescriptorSetMemory(uint32_t currentFrame)
    {
        //send the model matrix to the gpu
        entities::ModelMatrixUniformBuffer modelMatrixUniformBuffer;
        modelMatrixUniformBuffer.model = glm::mat4(1.0f);
        modelMatrixUniformBuffer.model *= glm::translate(glm::mat4(1.0f), mPosition);
        modelMatrixUniformBuffer.model *= glm::mat4_cast(mOrientation);
        void* modelMatUB = reinterpret_cast<void*>(mOffsets[currentFrame]);
        memcpy(modelMatUB, &modelMatrixUniformBuffer, sizeof(entities::ModelMatrixUniformBuffer));
    }
}
