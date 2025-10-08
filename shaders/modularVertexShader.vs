#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out vec3 FragPos;   // world-space position
out vec3 Normal;    // world-space normal
out vec2 TexCoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    // World-space position
    vec4 worldPos = model * vec4(aPos, 1.0);
    FragPos = vec3(worldPos);

    // Transform normal with inverse-transpose of model (world space)
    Normal = mat3(transpose(inverse(model))) * aNormal;

    TexCoords = aTexCoords;

    // Project with view and projection
    gl_Position = projection * view * worldPos;
}
