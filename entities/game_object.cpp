#include "game_object.h"
#include <map>
#include <vk\descriptor_service.h>
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
    GameObect::GameObect(const std::string& name, vk::DescriptorService& descriptorService, entities::Mesh* mesh)
        :mName(name), mId(GetNextGameObjectId()), mDescriptorService(descriptorService), mMesh(mesh)
    {
        //Get the descriptor set for model matrix
        mDescriptorSets = mDescriptorService.DescriptorSet(vk::OBJECT_LAYOUT_NAME, mId);
        mOffsets = mDescriptorService.DescriptorSetsBuffersOffsets(vk::OBJECT_LAYOUT_NAME, mId);
    }
}
