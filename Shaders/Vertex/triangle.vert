#version 450 

// Object space position
layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inColor;
layout (location = 2) in vec2 inTexCoord;
// Object space normal
layout (location = 3) in vec3 inNormal;


layout (location = 0) out vec3 fragColor;
layout (location = 1) out vec2 fragTexCoord;
layout (location = 2) out vec3 normalCamspace;
layout (location = 3) out vec3 postionCamspace;

layout (binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

void main() {
    fragColor = inColor;
    fragTexCoord = inTexCoord;
    normalCamspace = (ubo.view * ubo.model * vec4(inNormal, 1.0)).xyz;
    postionCamspace = (ubo.view * ubo.model * vec4(inPosition, 1.0)).xyz;
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);
}