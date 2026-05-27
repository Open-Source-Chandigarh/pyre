#version 420 core

layout (location = 0) in vec3 aPos;

#include "../includes/globalUbos.glsl"

uniform mat4 model;

void main()
{
    gl_Position = proj * view * model * vec4(aPos, 1.0);
}