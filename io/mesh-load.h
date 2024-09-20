#pragma once
#include <memory>
#include "entities/mesh-data.h"
namespace io {
    /// <summary>
    /// Remember that a file can have many meshes.
    /// </summary>
    /// <param name="file"></param>
    /// <returns></returns>
    std::vector<std::shared_ptr<entities::MeshData>> LoadMeshes(
        const std::string& file
    );
}