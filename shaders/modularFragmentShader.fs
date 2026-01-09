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
        FragColor = vec4(1, 0, 1, 1); // Magic pink: texture is not bound
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

    // Calculate reflectivity if present
    vec3 reflectionColor = vec3(0.0);

    if(material_skybox_present != 0 && GetShininess() > 0)
    {
        vec3 I = normalize(FragPos - vec3(viewPos));
        vec3 R = reflect(I, N);
        reflectionColor = texture(material_skybox, R).rgb;
    }

    vec3 finalColor = mix(result, reflectionColor, material_reflectivity);

    // Alpha from diffuse texture if present
    float alpha = 1.0;
    if (material_diffuse_present == 1) {
        alpha = texture(material_diffuse, TexCoords).a;
        if (alpha < 0.05) discard; // alpha cutoff
    }
    
    FragColor = vec4(finalColor, alpha);
}