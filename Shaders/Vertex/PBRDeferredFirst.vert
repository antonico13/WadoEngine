#version 450 

// Object space position
layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inColor;
layout (location = 2) in vec3 inNormal;
// Texture coordinate used for all the material maps
layout (location = 3) in vec2 inTexCoord;
layout (location = 4) in uint inTexIndex;

layout (location = 0) out vec2 fragTexCoord;
layout (location = 1) out vec3 posWorldSpace;
layout (location = 2) out float outBaseFresnel;
layout (location = 3) out uint outTexIndex;

layout (binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo; 

void main() {
    fragTexCoord = inTexCoord;
    outBaseFresnel = 0.05;
    posWorldSpace = inPosition; // (ubo.view * ubo.model * vec4(inPosition, 1.0)).xyz;
    outTexIndex = inTexIndex;
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);
}