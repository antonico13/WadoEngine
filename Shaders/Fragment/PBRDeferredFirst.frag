#version 450

// xyz = base diffuse color, w = roughness 
layout (location = 0) out vec4 diffuseProperties;
// xyz = normal, w = reflectance 
layout (location = 1) out vec4 specularProperties; 
// x = ambient occlusion, y = metallic properties, zw = other flags 
layout (location = 2) out vec4 meshProperties;
layout (location = 3) out vec4 positionProperties; 

layout (location = 0) in vec2 fragTexCoord;
layout (location = 1) in vec3 posCamspace;  
layout (location = 2) in float baseFresnel; // reflectance? 

// at some point could probably combine some of these to save even more space?
layout (binding = 1) uniform sampler2D diffuseSampler;
layout (binding = 2) uniform sampler2D normalMapSampler;
layout (binding = 3) uniform sampler2D metallicMapSampler;
layout (binding = 4) uniform sampler2D roughnessMapSampler;
layout (binding = 5) uniform sampler2D aoMapSampler;
// layout (binding = 6) uniform sampler2D emissiveSampler/subSurfaceSampler at some point ;

void main() {
    // First G-Buffer 
    diffuseProperties.xyz = texture(diffuseSampler, fragTexCoord).xyz;
    diffuseProperties.w = texture(roughnessMapSampler, fragTexCoord).x;

    // Second G-Buffer
    specularProperties.xyz = normalize(texture(normalMapSampler, fragTexCoord).xyz);
    specularProperties.w = baseFresnel;
    
    meshProperties.x = texture(aoMapSampler, fragTexCoord).x;
    meshProperties.y = texture(metallicMapSampler, fragTexCoord).x;
    // Nothing yet. 
    meshProperties.zw = vec2(1.0, 1.0);

    positionProperties.xyz = posCamspace;
    positionProperties.w = 1.0;
}