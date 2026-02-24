#version 420 core
layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 BrightColor;

in vec2 TexCoords;

layout (binding = 5) uniform sampler2D gPosition;
layout (binding = 6) uniform sampler2D gNormal;
layout (binding = 7) uniform sampler2D gAlbedoSpec;
layout (binding = 15) uniform samplerCube skyboxTexture;

#include "../includes/globalUbos.glsl"
// these are not used in deferred Lighting, but must exist 
// because lightingCommon.glsl references them in unused functions
uniform sampler2D material_diffuse;
uniform samplerCube material_skybox;
uniform sampler2D material_specular;
uniform sampler2D material_normal;
uniform sampler2D material_displacement;

uniform float material_shininess;
uniform float material_reflectivity;
uniform int material_diffuse_present;
uniform int material_specular_present;
uniform int material_skybox_present;
uniform int material_normal_present;
uniform int material_displacement_present;

vec3 GetDiffuseColor(vec2 uv) 
{
    return texture(gAlbedoSpec, uv).rgb;
}

vec3 GetSpecularColor(vec2 uv) 
{
    // we stored metallic/spec intensity in the alpha channel of albedo
    return vec3(texture(gAlbedoSpec, uv).a);
}

float GetShininess() {
    // reverse the roughness calculation stored in gNormal.a
    float roughness = texture(gNormal, TexCoords).a;
    return (1.0 - roughness) * 256.0;
}

#include "../includes/lightingCommon.glsl"

void main()
{
    // get base data from gBuffer geometry pass
    vec3 fragPos = texture(gPosition, TexCoords).rgb;
    vec3 normal = texture(gNormal, TexCoords).rgb;
    float depth = texture(gPosition, TexCoords).a;
    float reflectivity = texture(gAlbedoSpec, TexCoords).a;

    if (depth <= 0.0 || depth > 200.0)
        discard; 

    vec3 viewDir = normalize(viewPos - fragPos);
    vec3 lightingResult = vec3(0.0);

    lightingResult += CalcDirLight(normal, fragPos, viewDir, TexCoords);

    for (int i = 0; i < numPointLights; ++i)
        lightingResult += CalcPointLight(i, normal, fragPos, viewDir, TexCoords);

    for (int i = 0; i < numSpotLights; ++i)
        lightingResult += CalcSpotLight(i, normal, fragPos, viewDir, TexCoords);

    vec3 reflectionColor = vec3(0.0);
    float rawReflectivity = texture(gAlbedoSpec, TexCoords).a;
    float specIntensity = abs(rawReflectivity);
    bool wantsEnvMap = (rawReflectivity < 0.0); // if reflectivity is negative that means we want env mapping since we set it negative in geomtry pass if skybox was true

    if(wantsEnvMap && specIntensity > 0.01)
    {
        vec3 I = normalize(fragPos - viewPos);
        vec3 R = reflect(I, normal);
        reflectionColor = texture(skyboxTexture, R).rgb;
    }

    vec3 finalColor = mix(lightingResult, reflectionColor, (wantsEnvMap ? specIntensity : 0.0));
    FragColor = vec4(finalColor, 1.0);

    float brightness = dot(FragColor.rgb, vec3(0.2126, 0.7152, 0.0722));
    if(brightness > 1.0)
        BrightColor = vec4(FragColor.rgb, 1.0);
    else
        BrightColor = vec4(0.0, 0.0, 0.0, 1.0);
}