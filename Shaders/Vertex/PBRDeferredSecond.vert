#version 450

// Texture coordinates that will be interpolated and used to access the G-Buffers in the fragment pass
layout (location = 0) out vec2 fragTexCoord;

// Main program for each vertex of the render quad
// Taken from https://www.saschawillems.de/blog/2016/08/13/vulkan-tutorial-on-rendering-a-fullscreen-quad-without-buffers/
void main() {
  //gl_Position = vec4(inVertexPos, 1.0);
  fragTexCoord = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
  gl_Position = vec4(fragTexCoord * 2.0f -1.0f, 0.0f, 1.0f);
}