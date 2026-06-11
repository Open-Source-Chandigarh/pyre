#version 420 core

layout (location = 0) in vec3 aPos;

out vec3 localPos;

#include "includes/globalUbos.glsl"

void main()
{
    localPos = aPos;
    gl_Position = proj * view * vec4(localPos, 1.0);
}