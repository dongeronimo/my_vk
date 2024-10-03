#version 450
//REMEBER, this is actually the lightViewProj
layout(set = 0, binding = 0) uniform CameraUniformBuffer {
    mat4 view;
    mat4 proj;
    vec3 cameraPos;
} cameraUniform;
//The model matrix
layout(set = 1, binding = 0) uniform ObjectUniformBuffer {
    mat4 model;
} objectUniform;

layout(location=0) in vec3 inPosition;
layout(location=1) in vec2 inUV0;
layout(location=2) in vec3 inNormal;

layout(location = 0) out float fragDepth;

void main() {
    vec4 worldPosition = objectUniform.model * vec4(inPosition, 1.0);
    vec4 viewPosition = cameraUniform.view * worldPosition;
    gl_Position = cameraUniform.proj * viewPosition;
    fragDepth = gl_Position.z / gl_Position.w;// Normalized device coordinates depth
}