#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform float outlineWidth;

void main()
{
    vec3 pos = aPos;
    if (outlineWidth > 0.0) {
        pos += aNormal * outlineWidth;
    }
    gl_Position = projection * view * model * vec4(pos, 1.0);
}