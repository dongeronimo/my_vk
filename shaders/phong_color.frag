#version 450
layout (set = 2, binding = 0) uniform DirectionalLightUniformBuffer{
    vec3 direction[16];
    vec4 diffuseColorAndIntensity[16]; //TODO phong: Separate diffuse data from specular data
} directionalLight;
layout (set = 2, binding = 1) uniform AmbientLightUniformBuffer {
    vec4 diffuseColorAndIntensity;
} ambientLight;

layout (location=0) out vec4 outColor;

layout (location=0) in vec4 inColor;
layout (location=1) in vec3 inFragPos;
layout (location=2) in vec3 inNormal;
layout (location=3) in vec3 inCP;
//TODO phong: use multiple lights
void main(){
    //the ambient component
    vec3 ambient = ambientLight.diffuseColorAndIntensity.rgb * ambientLight.diffuseColorAndIntensity.a;
    //diffuse component
    vec3 norm = normalize(inNormal);
    vec3 lightDir = normalize(-directionalLight.direction[0]);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * directionalLight.diffuseColorAndIntensity[0].rgb * directionalLight.diffuseColorAndIntensity[0].a;
    //specular component TODO lightning: do specular
    vec3 viewDir = normalize(inCP - inFragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    vec3 specular = spec * directionalLight.diffuseColorAndIntensity[0].rgb * directionalLight.diffuseColorAndIntensity[0].a; 
    //combine results
    vec3 phong = ambient + diffuse + specular;
    outColor = vec4(phong * inColor.rgb, inColor.a);
}