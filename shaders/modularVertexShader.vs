#version 420 core
// Vertex shader: uses global UBO names view/proj defined in global_ubos.glsl
#include "includes/globalUbos.glsl"

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

// A mat4 is treated as 4 vec4s by the driver
// Instancing Matrix (Takes up locations 3, 4, 5, 6)
layout (location = 3) in mat4 aInstanceMatrix;

#ifdef HAS_GEOMETRY_SHADER
    // If GS exists, package outputs into a struct
    out VS_OUT {
        vec3 FragPos;
        vec3 Normal;
        vec2 TexCoords;
    } vs_out;
#else
    // If NO GS, output standard globals for the FS
    out vec3 FragPos;
    out vec3 Normal;
    out vec2 TexCoords;
#endif

uniform mat4 model;        // Used when isInstanced = false
uniform bool isInstanced;  // Model matrix Switch

void main()
{
    // If instanced, use the attribute. If not, use the uniform
    mat4 currentModel = isInstanced ? aInstanceMatrix : model;

    vec3 worldPos = vec3(currentModel * vec4(aPos, 1.0));
    vec3 worldNormal = mat3(transpose(inverse(currentModel))) * aNormal;
    
    #ifdef HAS_GEOMETRY_SHADER
        vs_out.FragPos = worldPos;
        vs_out.Normal = worldNormal;
        vs_out.TexCoords = aTexCoords;
    #else
        FragPos = worldPos;
        Normal = worldNormal;
        TexCoords = aTexCoords;
    #endif

    gl_Position = proj * view * vec4(worldPos, 1.0);
}