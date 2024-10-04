#version 450
layout(set = 0, binding = 0) uniform CameraUniformBuffer {
    mat4 view;
    mat4 proj;
    vec3 cameraPos;
} cameraUniform;

layout(set = 1, binding = 0) uniform ObjectUniformBuffer {
    mat4 model;
} objectUniform;

layout (set = 2, binding = 0) uniform DirectionalLightUniformBuffer{
    vec3 direction;
    vec4 diffuseColorAndIntensity;
    mat4 lightMatrix;
} directionalLight;

layout (push_constant) uniform PushConstants {
    vec4 color;
} colorPushConstant;

layout(location=0) in vec3 inPosition;
layout(location=1) in vec2 inUV0;
layout(location=2) in vec3 inNormal;

layout(location=0) out vec4 outColor;
layout(location=1) out vec3 outFragPos;
layout(location=2) out vec3 outNormal;
layout(location=3) out vec3 outCP;
layout(location=4) out vec2 outUV0;
layout(location=5) out vec4 fragPosLightSpace;
void main() {
    outUV0 = inUV0;
    outColor = colorPushConstant.color;
    outFragPos = vec3(objectUniform.model * vec4(inPosition,1.0));
    outCP = cameraUniform.cameraPos;
    outNormal = mat3(transpose(inverse(objectUniform.model))) * inNormal;

    vec4 worldPosition = objectUniform.model * vec4(inPosition, 1.0);
    fragPosLightSpace = directionalLight.lightMatrix * worldPosition;
    
    gl_Position = cameraUniform.proj * cameraUniform.view * objectUniform.model * vec4(inPosition, 1.0);
}