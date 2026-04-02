#version 420 core
#include "../includes/globalUbos.glsl"

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in mat4 aInstanceMatrix;

out vec3 FragPos;
out vec2 TexCoords;
out mat3 TBN;

uniform mat4 model;
uniform bool isInstanced;

void main()
{
    mat4 currentModel = isInstanced ? aInstanceMatrix : model;
    vec3 worldPos = vec3(currentModel * vec4(aPos, 1.0));
    
    mat3 normalMatrix = mat3(transpose(inverse(currentModel)));
    vec3 T = normalize(normalMatrix * aTangent);
    vec3 N = normalize(normalMatrix * aNormal);
    
    // gram-schmidt re-orthogonalization
    T = normalize(T - dot(T, N) * N);
    vec3 B = cross(N, T);
    
    FragPos = worldPos;
    TexCoords = aTexCoords;
    TBN = mat3(T, B, N);
    
    gl_Position = proj * view * vec4(worldPos, 1.0);
}