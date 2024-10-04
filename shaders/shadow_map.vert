#version 450

layout(set = 0, binding = 0) uniform DirectionalLightUniformBuffer {
    vec3 direction;
    vec4 diffuseColorAndIntensity;
    mat4 lightMatrix;
} directionalLight;
layout (set = 0, binding = 1) uniform AmbientLightUniformBuffer {
    vec4 diffuseColorAndIntensity;
} ambientLight;

layout(set = 1, binding = 0) uniform ObjectUniformBuffer {
    mat4 model;
} objectUniform;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inUV0;
layout(location = 2) in vec3 inNormal;

layout(location = 0) out float fragDepth;

void main() {
    vec4 worldPosition = objectUniform.model * vec4(inPosition, 1.0);
    vec4 lightSpacePosition = directionalLight.lightMatrix * worldPosition;
    gl_Position = lightSpacePosition;
    fragDepth = gl_Position.z / gl_Position.w; // Normalized device coordinates depth
}
