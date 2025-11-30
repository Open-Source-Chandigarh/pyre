#version 420 core

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

out vec4 FragColor;

#include "includes/globalUbos.glsl"
#include "includes/materialCommon.glsl"
#include "includes/lightingCommon.glsl"

void main()
{
    if (material_diffuse_present != 1) {
        FragColor = vec4(1, 0, 1, 1); // Magic pink: texture is NOT bound
    return;
    }
    vec3 N = normalize(Normal);
    vec3 V = normalize(vec3(viewPos) - FragPos); // viewPos from UBO

    vec3 result = vec3(0.0);

    // Directional light (single)
    result += CalcDirLight(N, FragPos, V);

    // Point lights
    for (int i = 0; i < numPointLights; ++i)
        result += CalcPointLight(i, N, FragPos, V);

    // Spot lights
    for (int i = 0; i < numSpotLights; ++i)
        result += CalcSpotLight(i, N, FragPos, V);

    // Alpha from diffuse texture if present
    float alpha = 1.0;
    if (material_diffuse_present == 1) {
        alpha = texture(material_diffuse, TexCoords).a;
        if (alpha < 0.05) discard; // alpha cutoff
    }

    FragColor = vec4(result, alpha);
}