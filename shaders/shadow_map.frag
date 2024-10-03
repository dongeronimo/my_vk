#version 450


layout(location = 0) in float fragDepth;
layout(location = 0) out vec4 outColor;

void main() {
    float depth = fragDepth * 0.5 + 0.5; // Convert from [-1, 1] to [0, 1] bc vk depth is [1,0]
    outColor = vec4(vec3(depth), 1.0); // Grayscale color
}
