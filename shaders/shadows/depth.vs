#version 420 core
layout (location = 0) in vec3 aPos;
layout (location = 3) in mat4 aInstanceMatrix;
// when drawing instanced, the model matrix comes in via attributes 3,4,5,6

uniform mat4 model;
uniform bool isInstanced;

void main()
{
    // if we are drawing instances (asteroids), use the attribute.
    // if we are drawing a single object (planet), use the uniform.
    mat4 currentModel = isInstanced ? aInstanceMatrix : model;
    // for csm:
    // usually we transform to light space here.
    // but for csm, the geometry shader needs the world position first
    // so it can multiply it by 4 different matrices.
    gl_Position = currentModel * vec4(aPos, 1.0);
}