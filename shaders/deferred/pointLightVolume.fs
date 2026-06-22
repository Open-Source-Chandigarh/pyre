#version 420 core

layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 BrightColor;

layout (binding = 5) uniform sampler2D gPosition;
layout (binding = 6) uniform sampler2D gNormal;
layout (binding = 7) uniform sampler2D gAlbedoSpec;
layout (binding = 14) uniform sampler2D gEmissive;

#include "../includes/globalUbos.glsl"

uniform int lightIndex; 

// dummy material uniforms required by lightingCommon.glsl to compile
uniform sampler2D material_diffuse;
uniform samplerCube material_skybox;
uniform sampler2D material_specular;
uniform sampler2D material_normal;
uniform float material_shininess;
uniform float material_reflectivity;
uniform int material_diffuse_present;
uniform int material_specular_present;
uniform int material_skybox_present;
uniform int material_normal_present;

// global UV for the helper functions
vec2 TexCoords; 

vec3 GetDiffuseColor(vec2 uv) { return texture(gAlbedoSpec, uv).rgb; }
vec3 GetSpecularColor(vec2 uv) { return vec3(abs(texture(gAlbedoSpec, uv).a)); }
float GetShininess() {
    float roughness = texture(gNormal, TexCoords).a;
    return max((1.0 - roughness) * 256.0, 0.001);
}

#include "../includes/lightingCommon.glsl"

void main()
{
    // calculate UV based on screen coordinates
    TexCoords = gl_FragCoord.xy / vec2(textureSize(gPosition, 0));

    float depth = texture(gPosition, TexCoords).a;
    if (depth <= 0.0) discard;

    vec3 fragPos = texture(gPosition, TexCoords).rgb;
    vec3 normal  = normalize(texture(gNormal, TexCoords).rgb);
    vec3 viewDir = normalize(vec3(viewPos) - fragPos);

    vec3 albedo = texture(gAlbedoSpec, TexCoords).rgb;
    float roughness = texture(gNormal, TexCoords).a;
    float rawReflectivity = texture(gAlbedoSpec, TexCoords).a;
    float metallic = abs(rawReflectivity);

    vec3 result = CalcPointLight(lightIndex, normal, fragPos, viewDir, albedo, roughness, metallic);

    FragColor = vec4(result, 1.0);

    float brightness = dot(result, vec3(0.2126, 0.7152, 0.0722));
    if (brightness > 1.0)
        BrightColor = vec4(result, 1.0);
    else
        BrightColor = vec4(0.0, 0.0, 0.0, 1.0);
}