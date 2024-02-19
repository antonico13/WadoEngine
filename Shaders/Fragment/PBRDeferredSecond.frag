#version 450

#define MAX_LIGHTS 128
#define PI 3.1415926538

const float alphaG = 1.5;

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

    vec4 diffuseProperties = subpassLoad(diffuseInput);
    vec4 specularProperties = subpassLoad(specularInput);
    vec4 meshProperties = subpassLoad(meshInput);

    vec3 baseColor = diffuseProperties.xyz; // diffuse color
    float roughness = diffuseProperties.w; // this is diffuseContribution 
    vec3 normal = specularProperties.xyz; // chill 
    float reflectance = specularProperties.w; // i think this is fresnel 
    float ao = meshProperties.x; // ambient occlusion, i'm guessing multiply everything by this to get shaded parts 
    float metallic = meshProperties.y; // this is shininess i think 
    vec3 posCamspace = subpassLoad(positionInput).xyz;
    //vec3 emissive; // this would be ambient, since it always emits something. 

    vec3 diffuseColor = light.enableDiffuse ? baseColor : vec3(0.0, 0.0, 0.0);
    //vec3 ambientColor = light.enableAmbient ? vec3(0.1, 0.0, 0.0) : vec3(0.0, 0.0, 0.0);
    // not sure what to do here 
    vec3 specularColor = light.enableSpecular ? vec3(1.0, 1.0, 1.0) : vec3(0.0, 0.0, 0.0);

    // Use Lambert diffuse, GGX specular 
    
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
    float fresnel = fresnelCoefficientSchlick(lightDir, halfwayDir, reflectance);

    float ni = max(dot(normal, lightDir), 0.0);
    float no = max(dot(normal, viewDir), 0.0);

    // make sure we never divide by 0 
    float specularFactor = 4 * ni * no;
    specularFactor = specularFactor > 0.0 ? (1.0 / specularFactor) : 0.0;

    vec3 ggxContribution = (fresnel * shadowMask * distribution * specularFactor) * specularColor; 

    outColor = ao * vec4(lightContribution * ni * (roughness * lambertContribution + metallic * ggxContribution), 1.0);
}