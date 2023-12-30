#version 450

#define MAX_LIGHTS 128
#define SHININESS 16

/*struct PointLight {
    vec3 lightPos;
    vec3 lightColor;
    float lightPower;
};*/

layout (location = 0) out vec4 outColor;

layout (location = 0) in vec3 fragColor;
layout (location = 1) in vec2 fragTexCoord;
layout (location = 2) in vec3 normalCamspace;
layout (location = 3) in vec3 posCamspace;

layout (binding = 1) uniform sampler2D texSampler;
layout (binding = 2) uniform PointLight {
                        vec3 lightPos; // assume this is given in cam space
                        vec3 lightColor;
                        float lightPower; 
                    } light;

void main() {
    vec3 diffuseColor = texture(texSampler, fragTexCoord).xyz;
    vec3 ambientColor = vec3(0.1, 0.0, 0.0);
    vec3 specularColor = vec3(1.0, 1.0, 1.0);

    vec3 normal = normalize(normalCamspace);
    vec3 lightDir = light.lightPos - posCamspace;
    vec3 viewDir = -normalize(posCamspace);
    float lightDistance = length(lightDir);
    lightDir = normalize(lightDir);

    float diffuseCoefficient = max(dot(lightDir, normal), 0.0);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float specularAngle = max(dot(halfwayDir, normal), 0.0);
    float specularCoefficient = pow(specularAngle, SHININESS);
    
    vec3 color = ambientColor + ((diffuseColor * diffuseCoefficient * + specularColor * specularCoefficient) * light.lightColor * light.lightPower) / (lightDistance * lightDistance);
    outColor = vec4(color, 1.0); 
}