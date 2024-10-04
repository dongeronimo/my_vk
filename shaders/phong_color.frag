#version 450
layout (set = 2, binding = 0) uniform DirectionalLightUniformBuffer{
    vec3 direction;
    vec4 diffuseColorAndIntensity;
    mat4 lightMatrix;
} directionalLight;

layout (set = 2, binding = 1) uniform AmbientLightUniformBuffer {
    vec4 diffuseColorAndIntensity;
} ambientLight;

layout(set = 3, binding = 0) uniform sampler2D colorSampler;
layout(set = 3, binding = 1) uniform sampler2D depthSampler;

layout (location=0) out vec4 outColor;

layout (location=0) in vec4 inColor;
layout (location=1) in vec3 inFragPos;
layout (location=2) in vec3 inNormal;
layout (location=3) in vec3 inCP;
layout (location=4) in vec2 inUV0;
layout (location=5) in vec4 fragPosLightSpace;
//TODO phong: use multiple lights
void main(){
    // Transform fragment position to light space
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5; // Transform to [0, 1] range
     // Sample depth from shadow map
    float closestDepth = texture(colorSampler, projCoords.xy).r;
    // Current depth from light's perspective
    float currentDepth = projCoords.z;
    // Shadow calculation
    float shadow = currentDepth > closestDepth + 0.005 ? 0.5 : 1.0;// Adding a small bias to prevent shadow acne
    
    //the ambient component
    vec3 ambient = ambientLight.diffuseColorAndIntensity.rgb * ambientLight.diffuseColorAndIntensity.a;
    //diffuse component
    vec3 norm = normalize(inNormal);
    vec3 lightDir = normalize(-directionalLight.direction);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * directionalLight.diffuseColorAndIntensity.rgb * directionalLight.diffuseColorAndIntensity.a;
    //specular component TODO lightning: do specular
    vec3 viewDir = normalize(inCP - inFragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    vec3 specular = spec * directionalLight.diffuseColorAndIntensity.rgb * directionalLight.diffuseColorAndIntensity.a; 
    //combine results
    vec3 phong = (ambient + diffuse + specular) * shadow;
    outColor = vec4(phong * inColor.rgb, inColor.a);

//    vec4 color = texture(colorSampler, inUV0);
//    float depth = texture(depthSampler, inUV0).r; // Assuming depth is stored in the red channel
//    //the ambient component
//    vec3 ambient = ambientLight.diffuseColorAndIntensity.rgb * ambientLight.diffuseColorAndIntensity.a;
//    //diffuse component
//    vec3 norm = normalize(inNormal);
//    vec3 lightDir = normalize(-directionalLight.direction);
//    float diff = max(dot(norm, lightDir), 0.0);
//    vec3 diffuse = diff * directionalLight.diffuseColorAndIntensity.rgb * 
//        directionalLight.diffuseColorAndIntensity.a;
//    //specular component TODO lightning: do specular
//    vec3 viewDir = normalize(inCP - inFragPos);
//    vec3 reflectDir = reflect(-lightDir, norm);
//    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
//    vec3 specular = spec * directionalLight.diffuseColorAndIntensity.rgb * 
//    directionalLight.diffuseColorAndIntensity.a; 
//    //combine results
//    vec3 phong = ambient + diffuse + specular;
//    outColor = vec4(phong * color.rgb, color.a);
}