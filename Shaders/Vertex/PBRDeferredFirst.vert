#version 450 

// Object space position
layout (location = 0) in vec3 inPosition;
// Texture coordinate used for all the material maps
layout (location = 1) in vec2 inTexCoord;

layout (location = 1) out vec2 fragTexCoord;
layout (location = 2) out vec3 postionCamspace;

layout (binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

void main() {
    fragTexCoord = inTexCoord;
    postionCamspace = (ubo.view * ubo.model * vec4(inPosition, 1.0)).xyz;
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);
}