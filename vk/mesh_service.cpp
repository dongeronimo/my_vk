#include "mesh_service.h"
#include <io/mesh-load.h>
#include <utils/hash.h>
namespace vk {
    MeshService::MeshService(const std::vector<std::string>& assets)
    {
        for (auto a : assets) {
            auto meshesDataFromFile = io::LoadMeshes(a);
            for (auto& b : meshesDataFromFile) {
                entities::Mesh* mesh = new entities::Mesh(*b);
                auto hash = utils::Hash(a);
                assert(mMeshes.count(hash) == 0);
                mMeshes.insert({
                    hash,
                    mesh
                    });
            }
        }
    }
    entities::Mesh* MeshService::GetMesh(const std::string& n) const
    {
        auto hash = utils::Hash(n);
        return mMeshes.at(hash);
    }
    MeshService::~MeshService()
    {
        for (auto& kv : mMeshes) {
            delete kv.second;
        }
        mMeshes.clear();
    }
}
