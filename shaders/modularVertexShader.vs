#version 420 core
// Vertex shader: uses global UBO names view/proj defined in global_ubos.glsl
#include "includes/globalUbos.glsl"

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;

uniform mat4 model;

void main()
{
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;
    TexCoords = aTexCoords;

    // use view/proj directly from the UBO
    gl_Position = proj * view * vec4(FragPos, 1.0);
}