#pragma once
#include <vector>
#include <string>
#include <map>
#include <memory>
#include "entities/mesh.h"
#include "utils\hash.h"
namespace vk
{
    class MeshService {
    public:
        MeshService(const std::vector<std::string>& assets);
        entities::Mesh* GetMesh(const std::string& n)const;
        ~MeshService();
    private:
        std::map<hash_t, entities::Mesh*> mMeshes;
    };
}