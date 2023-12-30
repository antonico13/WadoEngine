#version 450

#define MAX_LIGHTS 128
#define SHININESS 20
#define PI 3.1415926538

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
layout (location = 4) in float alphaG;
layout (location = 5) in float baseFresnel;
layout (location = 6) in float diffuseSpecularRatio;

layout (binding = 1) uniform sampler2D texSampler;
layout (binding = 2) uniform PointLight {
                        vec3 lightPos; // assume this is given in cam space
                        vec3 lightColor;
                        float lightPower; 
                        bool enableDiffuse;
                        bool enableSpecular;
                        bool enableAmbient;
                    } light;


// calculate the tan square of an angle given its cosine
float tanSquared(float cosine) {
    return 1.0 / (cosine * cosine) - 1.0;
}

// definition from Walter et al 
// Pre - v, m and n are normalized 
float shadowMaskGGX(vec3 v, vec3 m, vec3 n, float alphaG) {
    float cosvn = dot(n, v); // maybe should make sure this > 0.0 at all times?
    float cosvm = dot(m, v);
    float maskFilter = max(cosvm / cosvn, 0.0);
    // tan^2 of theta v, where theta v is the angle between n and v 
    return (maskFilter * 2.0) / (1.0 + sqrt(1.0 + alphaG * alphaG * tanSquared(cosvn)));
}

// calculate 
float micronormalDistributionGGX(vec3 m, vec3 n, float alphaG) {
    float cosmn = dot(m, n);
    float distFilter = max(cosmn, 0.0);
    return (alphaG * alphaG * distFilter) / (PI * pow(cosmn, 4.0) * (alphaG * alphaG + tanSquared(cosmn)));
}

float fresnelCoefficientSchlick(vec3 i, vec3 m, float f) {
    float u = dot(i, m);
    return f + (1.0 - f) * pow(1.0 - u, 5.0);
}


void main() {
    vec3 diffuseColor = light.enableDiffuse ? texture(texSampler, fragTexCoord).xyz : vec3(0.0, 0.0, 0.0);
    vec3 ambientColor = light.enableAmbient ? vec3(0.1, 0.0, 0.0) : vec3(0.0, 0.0, 0.0);
    vec3 specularColor = light.enableSpecular ? vec3(1.0, 1.0, 1.0) : vec3(0.0, 0.0, 0.0);

    // Use Lambert diffuse, GGX specular 
    
    vec3 normal = normalize(normalCamspace); // this is n 
    vec3 lightDir = light.lightPos - posCamspace; // this is i 
    float lightDistance = length(lightDir);
    lightDir = normalize(lightDir);
    vec3 viewDir = normalize(-posCamspace); // this is o
    vec3 halfwayDir = normalize(lightDir + viewDir); // hr

    // contribution of the light 
    vec3 lightContribution = light.lightColor * light.lightPower / (lightDistance * lightDistance);

    vec3 lambertContribution = diffuseColor;

    
    // calculate specular contributions now 
    // GGX shadow mask, distribution, Schlick fresnel 
    float shadowMask = shadowMaskGGX(lightDir, halfwayDir, normal, alphaG) * shadowMaskGGX(viewDir, halfwayDir, normal, alphaG);
    float distribution = micronormalDistributionGGX(halfwayDir, normal, alphaG);
    float fresnel = fresnelCoefficientSchlick(lightDir, halfwayDir, baseFresnel);

    float ni = max(dot(normal, lightDir), 0.0);
    float no = max(dot(normal, viewDir), 0.0);

    // make sure we never divide by 0 
    float specularFactor = 4 * ni * no;
    specularFactor = specularFactor > 0.0 ? (1.0 / specularFactor) : 0.0;

    vec3 ggxContribution = (fresnel * shadowMask * distribution * specularFactor) * specularColor;

    outColor = vec4(ambientColor + lightContribution * ni * (diffuseSpecularRatio * diffuseColor + (1.0 - diffuseSpecularRatio) * ggxContribution), 1.0);
}