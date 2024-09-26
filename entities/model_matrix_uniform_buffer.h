#pragma once
#include <glm/glm.hpp>
namespace entities {
    /// <summary>
    /// Holds the data for the camera. The camera tends to be the same for all
    /// objects in the scene. 
    /// </summary>
    struct alignas(16) ModelMatrixUniformBuffer  {
        alignas(16)glm::mat4 model;
    };
}