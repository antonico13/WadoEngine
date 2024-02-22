#version 450

#define MAX_LIGHTS 128
#define PI 3.1415926538

layout (location = 0) out vec4 outColor;

layout (location = 0) in vec2 fragTexCoord; // all maps have the same size (?), use one texture coordinate for everything 
// for smaller maps its fine I think depends on how I write the sampler to work 


// xyz = base diffuse color, w = roughness 
//layout (binding = 0) uniform sampler2D materialSamplers[4]; // I'm sampling from images here in the framebuffer, not textures...
layout (input_attachment_index = 0, set = 0, binding = 0) uniform subpassInput diffuseInput;
layout (input_attachment_index = 1, set = 0, binding = 1) uniform subpassInput specularInput; 
layout (input_attachment_index = 2, set = 0, binding = 2) uniform subpassInput meshInput; 
layout (input_attachment_index = 3, set = 0, binding = 3) uniform subpassInput positionInput; 

// xyz = normal, w = reflectance 
/*layout (binding = 1) uniform sampler2D specularSampler; 
// x = ambient occlusion, y = metallic properties, zw = other flags 
layout (binding = 2) uniform sampler2D meshSampler; 
layout (binding = 3) uniform sampler2D positionSampler;*/
layout (binding = 4) uniform PointLight {
                        vec3 lightPos; // this is in world space 
                        vec3 lightColor;
                        float lightPower; 
                        bool enableDiffuse;
                        bool enableSpecular;
                        bool enableAmbient;
                    } light;

layout (binding = 5) uniform CameraProperties {
                        vec3 cameraPos;
                    } camera;

// definition from Brian Karis Epic Games 
// Pre - v, m and n are normalized 
float shadowMaskGGX(vec3 v, vec3 n, float k) {
    float cosvn = max(dot(n, v), 0.0); 
    return cosvn / (cosvn * (1.0 - k) + 1.0);
}

// calculate micronormal distribution 
// GGX, adapted by Brian Karis, Epic Games 
float micronormalDistributionGGX(vec3 h, vec3 n, float alphaG) {
    float alphaSq = alphaG * alphaG;
    return alphaSq / pow(PI * (pow( max(dot(n, h), 0.0),  2.0) * (alphaSq - 1.0) + 1.0), 2.0);
}

// From Schlick 93 
float fresnelCoefficientSchlick(vec3 i, vec3 m, float f) {
    float u = max(dot(i, m), 0.0);
    return f + (1.0 - f) * pow(1.0 - u, 5.0);
}

vec3 shiftNormal(vec3 normal) {
    return 0.5 * normal + 0.5;
}


void main() {

    vec4 diffuseProperties = subpassLoad(diffuseInput);
    vec4 specularProperties = subpassLoad(specularInput);
    vec4 meshProperties = subpassLoad(meshInput);

    vec3 baseColor = diffuseProperties.xyz; // diffuse color
    float roughness = diffuseProperties.w;
    vec3 normal = normalize(shiftNormal(specularProperties.xyz)); 
    float reflectance = specularProperties.w; // base fresnel 
    float ao = meshProperties.x; // ambient occlusion
    float metallic = meshProperties.y; // this is shininess (?)
    vec3 position = subpassLoad(positionInput).xyz;
    //vec3 emissive; // this would be ambient, since it always emits something. 

    vec3 diffuseColor = light.enableDiffuse ? baseColor : vec3(0.0, 0.0, 0.0);
    //vec3 ambientColor = light.enableAmbient ? vec3(0.1, 0.0, 0.0) : vec3(0.0, 0.0, 0.0);
    // right now light is white, only one per scene. This would need to be the color from the light itself 
    vec3 specularColor = light.enableSpecular ? vec3(1.0, 1.0, 1.0) : vec3(0.0, 0.0, 0.0);

    // Use Lambert diffuse, GGX specular 
    
    vec3 lightDir = light.lightPos - position; // this is l
    float lightDistance = length(lightDir);
    lightDir = normalize(lightDir);
    vec3 viewDir = normalize(camera.cameraPos - position); // this is v
    vec3 halfwayDir = sign(dot(normal, lightDir)) * normalize(lightDir + viewDir); // h

    // contribution of the light 
    float lightContribution = light.lightPower / (lightDistance * lightDistance);

    vec3 lambertContribution = diffuseColor / PI; // Brian Karis, Epic Games 

    
    // calculate specular contributions now 
    // GGX shadow mask, microfacet distribution, Schlick fresnel 
    float shadowMaskK = (roughness + 1.0) * (roughness + 1.0) / 8.0;
    float shadowMask = shadowMaskGGX(lightDir, normal, shadowMaskK) * shadowMaskGGX(viewDir, normal, shadowMaskK);
    float distribution = micronormalDistributionGGX(halfwayDir, normal, roughness * roughness);
    float fresnel = fresnelCoefficientSchlick(lightDir, halfwayDir, reflectance); // should this be the view vector instead here?

    float cosnl = max(dot(normal, lightDir), 0.0);
    float cosnv = max(dot(normal, viewDir), 0.0);

    // make sure we never divide by 0 
    float specularFactor = 4 * cosnl * cosnv;
    specularFactor = specularFactor > 0.0 ? (1.0 / specularFactor) : 0.0;

    vec3 ggxContribution = (fresnel * shadowMask * distribution * specularFactor) * specularColor; 
    outColor = vec4(diffuseColor, 1.0);
}