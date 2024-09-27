#version 450
layout (set = 2, binding = 0) uniform DirectionalLightUniformBuffer{
    vec3 direction[16];
    vec4 colorAndIntensity[16];
} directionalLight;
layout (set = 2, binding = 1) uniform AmbientLightUniformBuffer {
    vec4 colorAndIntensity;
} ambientLight;

layout (location=0) out vec4 outColor;
layout (location=0) in vec4 inColor;
layout (location=1) in vec3 inFragPos;
layout (location=2) in vec3 inNormal;

void main(){
    outColor = inColor;
}