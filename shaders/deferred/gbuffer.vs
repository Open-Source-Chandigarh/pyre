#version 420 core
#include "../includes/globalUbos.glsl"

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in mat4 aInstanceMatrix;

out vec3 FragPos;
out vec2 TexCoords;
out float LinearDepth;
out mat3 TBN;
out vec3 Normal;

uniform mat4 model;
uniform bool isInstanced;

void main()
{
    mat4 currentModel = isInstanced ? aInstanceMatrix : model;
    vec4 worldPos = currentModel * vec4(aPos, 1.0);
    vec4 viewSpacePos = view * worldPos;
    LinearDepth = -viewSpacePos.z; 
    FragPos = worldPos.xyz; 
    TexCoords = aTexCoords;

    mat3 normalMatrix = transpose(inverse(mat3(currentModel)));
    Normal = normalize(normalMatrix * aNormal);
    vec3 T = normalize(normalMatrix * aTangent);
    vec3 N = normalize(normalMatrix * aNormal);
    // gram-schmidt re-orthogonalization
    T = normalize(T - dot(T, N) * N);
    vec3 B = cross(N, T);
    TBN = mat3(T, B, N);

    gl_Position = proj * view * worldPos;
}