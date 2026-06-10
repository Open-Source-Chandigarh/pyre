#version 420 core
layout (location = 0) out vec4 gPosition; // A = Linear Depth
layout (location = 1) out vec4 gNormal;   // A = Roughness
layout (location = 2) out vec4 gAlbedo;   // A = Metallic

in vec3 FragPos;
in vec2 TexCoords;
in mat3 TBN;
in vec3 Normal;
in float LinearDepth;

#include "../includes/globalUbos.glsl"
#include "../includes/materialCommon.glsl"
#include "../includes/lightingCommon.glsl"

void main()
{
    // position and linear depth
    gPosition.rgb = FragPos;
    // store linear depth in alpha for generic post processing (ssao)
    gPosition.a = LinearDepth;

    vec3 V = normalize(vec3(viewPos) - FragPos); // viewDir in world space

    // Parallax Mapping
    vec2 uv = TexCoords;

    if(material_displacement_present == 1)
    {
        mat3 InvTBN = transpose(TBN); // since TBN is orthagonal it's transpose is equal to it's inverse
        // view dir in tangent space
        vec3 TangentViewDir = InvTBN * V;

        uv = ParallaxMapping(uv, TangentViewDir);

        if(uv.x > 1.0 || uv.y > 1.0 || uv.x < 0.0 || uv.y < 0.0)
            discard;
    }

    vec3 N = normalize(Normal);
    if (material_normal_present == 1)
    {
        vec3 mapNormal = texture(material_normal, uv).rgb;
        mapNormal = mapNormal * 2.0 - 1.0;
        N = normalize(TBN * mapNormal);
    }
    gNormal.rgb = N;

    float shininess = GetShininess();
    float roughness = 0.5;
    if (material_roughness_present == 1) 
    {
        roughness = texture(material_roughness, uv).r;
    } 
    else if (material_specular_present == 1) 
    {
        roughness = clamp(1.0 - (GetShininess() / 256.0), 0.05, 1.0); 
    }
    gNormal.a = roughness;

    vec3 albedo = material_diffuseColor;
    if (material_diffuse_present == 1)
    {
        vec4 texColor = texture(material_diffuse, uv);
        if(texColor.a < 0.05) discard; 
        albedo = texColor.rgb;
    }
    gAlbedo.rgb = albedo;

    float metallic = 0.0;
    if (material_metallic_present == 1) 
    {
        metallic = texture(material_metallic, uv).r;
    } 
    else if (material_specular_present == 1) 
    {
        metallic = GetSpecularColor(uv).r;
    }

    if (material_skybox_present == 1) // inverse sign to let lighting pass know that we want env mapping
        gAlbedo.a = -max(metallic, material_reflectivity); 
    else
        gAlbedo.a = max(metallic, material_reflectivity);
}