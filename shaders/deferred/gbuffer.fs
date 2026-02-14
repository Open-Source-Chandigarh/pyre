#version 420 core
layout (location = 0) out vec4 gPosition; // A = Linear Depth
layout (location = 1) out vec4 gNormal;   // A = Roughness
layout (location = 2) out vec4 gAlbedo;   // A = Metallic

in vec3 FragPos;
in vec2 TexCoords;
in mat3 TBN;
in vec3 Normal;
in float LinearDepth;

#include "../includes/materialCommon.glsl"

void main()
{
    // position and linear depth
    gPosition.rgb = FragPos;
    // store linear depth in alpha for generic post processing (ssao)
    gPosition.a = LinearDepth;

    vec3 N = normalize(Normal);
    if (material_normal_present == 1)
    {
        vec3 mapNormal = texture(material_normal, TexCoords).rgb;
        mapNormal = mapNormal * 2.0 - 1.0;
        N = normalize(TBN * mapNormal);
    }
    gNormal.rgb = N;

    float shininess = GetShininess();
    float roughness = clamp(1.0 - (shininess / 100.0), 0.05, 1.0);
    gNormal.a = roughness;

    vec3 albedo = material_diffuseColor;
    if (material_diffuse_present == 1)
    {
        vec4 texColor = texture(material_diffuse, TexCoords);
        if(texColor.a < 0.1) discard; 
        albedo = texColor.rgb;
    }
    gAlbedo.rgb = albedo;

    float metallic = 0.0;
    if (material_specular_present == 1)
    {
        metallic = texture(material_specular, TexCoords).r;
    }
    gAlbedo.a = metallic;
}