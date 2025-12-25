#version 420 core
layout (location = 0) in vec3 aPos;

uniform mat4 model;

void main()
{
    // note for csm:
    // usually we transform to light space here.
    // but for csm, the geometry shader needs the world position first
    // so it can multiply it by 4 different matrices.
    gl_Position = model * vec4(aPos, 1.0);
}