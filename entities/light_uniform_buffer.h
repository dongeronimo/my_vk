#pragma once
#include <glm/glm.hpp>
namespace entities {
    /// <summary>
    /// Uniform buffer for directional light
    /// </summary>
    struct alignas(16) DirectionalLightUniformBuffer {
        alignas(16) glm::vec3 direction[16];
        alignas(16) glm::vec4 colorAndIntensity[16];
    };
    /// <summary>
    /// uniform buffer for ambient light
    /// </summary>
    struct alignas(16) AmbientLightUniformBuffer {
        alignas(16) glm::vec4 colorAndIntensity;
    };
}