#version 450

#define MAX_TEXTURE_SETS 32

// xyz = base diffuse color, w = roughness 
layout (location = 0) out vec4 diffuseProperties;
// xyz = normal, w = reflectance 
layout (location = 1) out vec4 specularProperties; 
// x = ambient occlusion, y = metallic properties, zw = other flags 
layout (location = 2) out vec4 meshProperties;
layout (location = 3) out vec4 positionProperties; 

layout (location = 0) in vec2 fragTexCoord;
layout (location = 1) in vec3 posWorldSpace;  
layout (location = 2) in float baseFresnel; // reflectance? 
layout (location = 3) in flat uint inTexIndex;

// at some point could probably combine some of these to save even more space?
layout (binding = 1) uniform sampler2D diffuseSampler[MAX_TEXTURE_SETS];
layout (binding = 2) uniform sampler2D normalMapSampler[MAX_TEXTURE_SETS];
layout (binding = 3) uniform sampler2D metallicMapSampler[MAX_TEXTURE_SETS];
layout (binding = 4) uniform sampler2D roughnessMapSampler[MAX_TEXTURE_SETS];
layout (binding = 5) uniform sampler2D aoMapSampler[MAX_TEXTURE_SETS];
// layout (binding = 6) uniform sampler2D emissiveSampler/subSurfaceSampler at some point ;

void main() {
    // First G-Buffer 
    diffuseProperties.xyz = texture(diffuseSampler[inTexIndex], fragTexCoord).xyz;
    diffuseProperties.w = texture(roughnessMapSampler[inTexIndex], fragTexCoord).x;

    // Second G-Buffer
    specularProperties.xyz = normalize(texture(normalMapSampler[inTexIndex], fragTexCoord).xyz);
    specularProperties.w = baseFresnel;
    
    meshProperties.x = texture(aoMapSampler[inTexIndex], fragTexCoord).x;
    meshProperties.y = texture(metallicMapSampler[inTexIndex], fragTexCoord).x;
    // Nothing yet. 
    meshProperties.zw = vec2(inTexIndex, 1.0);

    positionProperties.xyz = posWorldSpace;
    positionProperties.w = 1.0;
}