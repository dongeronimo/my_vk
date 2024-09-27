#include "game_object.h"
#include <map>
#include "mesh.h"
#include <vk\descriptor_service.h>
#include <utils/hash.h>
#include "pipeline.h"
#include <vk/debug_utils.h>
#include "model_matrix_uniform_buffer.h"
#include "camera_uniform_buffer.h"
#include "vk\instance.h"
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
        gAvailableGameObjectsIds[mId] = false;
        //Get the descriptor sets
        //there are just MAX_FRAMES_IN_FLIGHT descriptor sets 
        mModelMatrixDescriptorSet = mDescriptorService.DescriptorSet(vk::MODEL_MATRIX_LAYOUT_NAME);

        //mModelMatrixOffset = mDescriptorService.DescriptorSetsBuffersAddrs(vk::MODEL_MATRIX_LAYOUT_NAME, mId);
        mCameraDescriptorSet = mDescriptorService.DescriptorSet(vk::CAMERA_LAYOUT_NAME, 0);//cameras, for now, are only id=0 because there is only one camera
        mCameraOffset = mDescriptorService.DescriptorSetsBuffersAddrs(vk::CAMERA_LAYOUT_NAME, 0);
    }

    void GameObject::Draw(VkCommandBuffer cmdBuffer, 
        Pipeline& pipeline, uint32_t currentFrame)
    {
        vk::SetMark({ 1.0f, 0.8f, 1.0f, 1.0f }, mName, cmdBuffer);
        CopyModelMatrixToDescriptorSetMemory(currentFrame);
        //bind the mesh.
        mMesh->Bind(cmdBuffer);
        //bind the descriptor sets
        vkCmdBindDescriptorSets(
            cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
            pipeline.PipelineLayout(),
            vk::CAMERA_SET,
            1,
            &mCameraDescriptorSet[currentFrame],
            0,
            nullptr
        );
        uint32_t modelMatrixDynamicOffset = vk::CalculateDynamicOffset<entities::ModelMatrixUniformBuffer>(currentFrame,
            vk::MAX_NUMBER_OF_OBJECTS, mId);
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

    GameObject::~GameObject()
    {
        gAvailableGameObjectsIds[mId] = true;
    }

    void GameObject::CopyModelMatrixToDescriptorSetMemory(uint32_t currentFrame)
    {
        uintptr_t bufferOffset = vk::CalculateDynamicOffset<entities::ModelMatrixUniformBuffer>(currentFrame,
            vk::MAX_NUMBER_OF_OBJECTS, mId);
        //Get the beginning of the buffer
        uintptr_t baseAddress = mDescriptorService.ModelMatrixBaseAddress();
        uintptr_t position = baseAddress + bufferOffset;
        //assembles the model matrix
        entities::ModelMatrixUniformBuffer modelMatrixUniformBuffer;
        modelMatrixUniformBuffer.model = glm::mat4(1.0f);
        modelMatrixUniformBuffer.model *= glm::translate(glm::mat4(1.0f), mPosition);
        modelMatrixUniformBuffer.model *= glm::mat4_cast(mOrientation);
        memcpy(reinterpret_cast<void*>(position), &modelMatrixUniformBuffer, sizeof(entities::ModelMatrixUniformBuffer));
    }
}
