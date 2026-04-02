#version 420 core
layout (location = 0) out vec4 gPosition;
layout (location = 1) out vec4 gNormal;
layout (location = 2) out vec4 gAlbedoSpec;

in vec3 FragPos;
in vec2 TexCoords;
in mat3 TBN;

#include "../includes/globalUbos.glsl"
#include "../includes/materialCommon.glsl"

float LinearizeDepth(float depth) 
{
    float z = depth * 2.0 - 1.0; // Back to NDC 
    return (2.0 * nearPlane * farPlane) / (farPlane + nearPlane - z * (farPlane - nearPlane));	// Reverse the projection matrix
}

vec2 ParallaxMapping(vec2 texCoords, vec3 viewDir)
{
    const float minLayers = 8.0;
    const float maxLayers = 32.0;
    float numLayers = mix(maxLayers, minLayers, max(dot(vec3(0.0, 0.0, 1.0), viewDir), 0.0));  
    float layerDepth = 1.0 / numLayers;
    float currentLayerDepth = 0.0;
    vec2 P = viewDir.xy * 0.03; 
    vec2 deltaTexCoords = P / numLayers;

    vec2 currentTexCoords = texCoords;
    float currentDepthMapValue = texture(material_displacement, currentTexCoords).r;

    while(currentLayerDepth < currentDepthMapValue)
    {
        currentTexCoords -= deltaTexCoords;
        currentDepthMapValue = texture(material_displacement, currentTexCoords).r;  
        currentLayerDepth += layerDepth;  
    }

    vec2 prevTexCoords = currentTexCoords + deltaTexCoords;
    float afterDepth  = currentDepthMapValue - currentLayerDepth;
    float beforeDepth = texture(material_displacement, prevTexCoords).r - currentLayerDepth + layerDepth;

    float weight = afterDepth / (afterDepth - beforeDepth);
    vec2 finalTexCoords = prevTexCoords * weight + currentTexCoords * (1.0 - weight);
    return finalTexCoords;
}

void main()
{
    vec2 uv = TexCoords;

    // Parallax Mapping
    if(material_displacement_present == 1)
    {
        vec3 V = normalize(vec3(viewPos) - FragPos);
        mat3 InvTBN = transpose(TBN); 
        vec3 TangentViewDir = InvTBN * V;
        
        uv = ParallaxMapping(uv, TangentViewDir);
        if(uv.x > 1.0 || uv.y > 1.0 || uv.x < 0.0 || uv.y < 0.0) discard;
    }

    // Alpha Cutoff
    float alpha = 1.0;
    if (material_diffuse_present == 1) {
        alpha = texture(material_diffuse, uv).a;
        if (alpha < 0.05) discard; 
    }

    // Output World Position + Linear Depth (in Alpha)
    gPosition.rgb = FragPos;
    gPosition.a = LinearizeDepth(gl_FragCoord.z) / farPlane; 

    // Output Normal + Shininess
    vec3 N = normalize(TBN[2]); 
    if (material_normal_present == 1)
    {
        vec3 normalMap = texture(material_normal, uv).rgb;
        normalMap = normalMap * 2.0 - 1.0;
        N = normalize(TBN * normalMap);
    }
    gNormal.rgb = N;
    gNormal.a = GetShininess() / 256.0; // Packed for Blinn-Phong

    // Output Albedo + Specular
    gAlbedoSpec.rgb = GetDiffuseColor(uv);
    gAlbedoSpec.a = GetSpecularColor(uv).r; 
}