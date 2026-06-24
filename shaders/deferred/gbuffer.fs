#version 420 core
layout (location = 0) out vec4 gNormal;   // A = Roughness
layout (location = 1) out vec4 gAlbedo;   // A = Metallic
layout (location = 2) out vec3 gEmissive;

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

    float roughness = clamp(1.0 - (GetShininess() / 256.0), 0.05, 1.0);
    if (material_roughness_present == 1) 
    {
        roughness *= texture(material_roughness, uv).g;
    } 

    // specular anti-aliasing (geometric variance)
    // calculate how fast the normal is changing from pixel to pixel
    vec3 dNdx = dFdx(N);
    vec3 dNdy = dFdy(N);
    
    // the scale factor dictates how aggressively roughness increases on edges
    float variance = 5.0 * (dot(dNdx, dNdx) + dot(dNdy, dNdy));
    
    // widen the specular lobe where the normal variance is high
    roughness = min(sqrt(roughness * roughness + variance), 1.0);

    gNormal.a = roughness;

    vec3 albedo = material_diffuseColor;
    if (material_diffuse_present == 1)
    {
        vec4 texColor = texture(material_diffuse, uv);
        if(texColor.a < 0.05) discard; 
        albedo = texColor.rgb;
    }
    gAlbedo.rgb = albedo;

    float metallic = material_reflectivity;
    if (material_metallic_present == 1) 
    {
        metallic *= texture(material_metallic, uv).b;
    }

    vec3 emission = material_emissiveColor;
    if (material_emissive_present == 1)
    {
        emission = texture(material_emissive, TexCoords).rgb;
        emission *= 10.0;
        emission *= emission_tint;
    }

    gEmissive = emission;

    if (material_skybox_present == 1) // inverse sign to let lighting pass know that we want env mapping
        gAlbedo.a = -metallic; 
    else
        gAlbedo.a = metallic;
}