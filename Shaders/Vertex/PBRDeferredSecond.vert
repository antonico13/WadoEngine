#version 450

// Render quad vertex coordinates
layout (location = 0) in vec3 inVertexPos;
// Texture coordinates for this vertex
layout (location = 1) in vec2 inTexCoord;

// Texture coordinates that will be interpolated and used to access the G-Buffers in the fragment pass
layout (location = 0) out vec2 fragTexCoord;

// Main program for each vertex of the render quad
void main() {
  gl_Position = vec4(inVertexPos, 1.0);
  fragTexCoord = inTexCoord;
}