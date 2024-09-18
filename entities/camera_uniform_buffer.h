#pragma once
#include <glm/glm.hpp>
namespace entities {
    /// <summary>
    /// Holds the data for the camera. The camera tends to be the same for all
    /// objects in the scene. 
    /// </summary>
    struct alignas(16) CameraUniformBuffer {
        /// <summary>
        /// View matrix 
        /// </summary>
        alignas(16)glm::mat4 view;
        /// <summary>
        /// Projection matrix
        /// </summary>
        alignas(16)glm::mat4 proj;
        /// <summary>
        /// Camera position
        /// </summary>
        alignas(16)glm::vec3 cameraPos;
    };
}