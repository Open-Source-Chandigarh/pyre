#version 420 core

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;
in mat3 TBN;

out vec4 FragColor;
out vec4 BrightColor;

#include "includes/globalUbos.glsl"
#include "includes/materialCommon.glsl"
#include "includes/lightingCommon.glsl"

void main()
{
    // view dir in world space
    vec3 V = normalize(vec3(viewPos) - FragPos); // viewPos from UBO

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

    // Normal Mapping
    vec3 N;
    if (material_normal_present == 1)
    {
        // Sample map [0,1]
        vec3 normal = texture(material_normal, uv).rgb;
        // Transform to [-1, 1]
        normal = normal * 2.0 - 1.0;
        // TBN Transform
        N = normalize(TBN * normal);
    }
    else
    {
        N = normalize(Normal);
    }

    float alpha = 1.0;
    if (material_diffuse_present == 1) 
    {
        alpha = texture(material_diffuse, uv).a;
        if (alpha < 0.05) discard; 
    }

    vec3 albedo = material_diffuseColor;
    if (material_diffuse_present == 1) 
    {
        albedo = texture(material_diffuse, uv).rgb;
    }

    float roughness = 0.5;
    if (material_roughness_present == 1) 
    {
        roughness = texture(material_roughness, uv).r;
    } 
    else if (material_specular_present == 1) 
    {
        roughness = clamp(1.0 - (GetShininess() / 256.0), 0.05, 1.0);
    }

    float metallic = 0.0;
    if (material_metallic_present == 1) 
    {
        metallic = texture(material_metallic, uv).r;
    } 
    else if (material_specular_present == 1) 
    {
        metallic = texture(material_specular, uv).r;
    }

    vec3 result = vec3(0.0);

    // Directional light (single)
    result += CalcDirLight(N, FragPos, V, uv, albedo, roughness, metallic, 1.0);

    // Point lights
    for (int i = 0; i < numPointLights; ++i)
        result += CalcPointLight(i, N, FragPos, V, albedo, roughness, metallic);

    // Spot lights
    for (int i = 0; i < numSpotLights; ++i)
        result += CalcSpotLight(i, N, FragPos, V, uv);

    // Calculate reflectivity if present
    vec3 reflectionColor = vec3(0.0);

    if(material_skybox_present != 0 && GetShininess() > 0)
    {
        vec3 I = normalize(FragPos - vec3(viewPos));
        vec3 R = reflect(I, N);
        reflectionColor = texture(material_skybox, R).rgb;
    }

    float reflectionMix = max(metallic, material_reflectivity);
    vec3 finalColor = mix(result, reflectionColor, material_skybox_present != 0 ? reflectionMix : 0.0);
    
    FragColor = vec4(finalColor, alpha);
    // convert to grayscale to get the perceived brightness of this fragment
    float brightness = dot(FragColor.rgb, vec3(0.2126, 0.7152, 0.0722));
    // if brightness is greater than threshold we store this fragment in brightcolor otherwise we skip it
    if(brightness > 1.0)
        BrightColor = vec4(FragColor.rgb, 1.0);
    else
        BrightColor = vec4(0.0, 0.0, 0.0, 1.0);
}