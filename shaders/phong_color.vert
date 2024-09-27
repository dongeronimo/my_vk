#version 450
layout(set = 0, binding = 0) uniform CameraUniformBuffer {
    mat4 view;
    mat4 proj;
    vec3 cameraPos;
} cameraUniform;
layout(set = 1, binding = 0) uniform ObjectUniformBuffer {
    mat4 model;
} objectUniform;

layout (push_constant) uniform PushConstants {
    vec4 color;
} colorPushConstant;

layout(location=0) in vec3 inPosition;
layout(location=1) in vec2 inUV0;
layout(location=2) in vec3 inNormal;

layout(location=0) out vec4 outColor;
layout(location=1) out vec3 outFragPos;
layout(location=2) out vec3 outNormal;
void main() {
    outColor = colorPushConstant.color;
    outFragPos = vec3(objectUniform.model * vec4(inPosition,1.0));
    outNormal = mat3(transpose(inverse(objectUniform.model)) * inNormal;
    gl_Position = cameraUniform.proj * cameraUniform.view * objectUniform.model * vec4(inPosition, 1.0);
}