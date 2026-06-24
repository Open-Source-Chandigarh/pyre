#version 420 core

layout (binding = 5) uniform sampler2D gDepth;
layout (binding = 6) uniform sampler2D gNormal;
layout (binding = 7) uniform sampler2D gAlbedoSpec;

out float FragColor;

#include "../includes/globalUbos.glsl"

uniform sampler2D texNoise;
uniform vec3 samples[64];
uniform float radius;

vec3 ReconstructPosition(vec2 uv, float depth) 
{
    float z = depth * 2.0 - 1.0; 
    vec4 ndcPos = vec4(uv * 2.0 - 1.0, z, 1.0); 
    vec4 worldPos = inverse(proj * view) * ndcPos; 
    return worldPos.xyz / worldPos.w; 
}

void main() 
{
    vec2 screenRes = vec2(textureSize(gDepth, 0));
    vec2 texCoords = gl_FragCoord.xy / screenRes;
    float depth = texture(gDepth, texCoords).r;
    if (depth == 1.0) discard;
    vec3 fragPosWorld = ReconstructPosition(texCoords, depth);
    vec3 fragNormalWorld = texture(gNormal, texCoords).rgb;

    vec3 fragPosView = (view * vec4(fragPosWorld, 1.0)).xyz;
    vec3 fragNormalView = normalize(mat3(view) * fragNormalWorld);

    vec2 noiseScale = screenRes / 4.0;
    vec3 randomVec = texture(texNoise, texCoords * noiseScale).xyz;

    // Gramm-Schmidt process
    vec3 tangent = normalize(randomVec - fragNormalView * dot(randomVec, fragNormalView));
    vec3 bitangent = cross(fragNormalView, tangent);

    mat3 TBN = mat3(tangent, bitangent, fragNormalView);

    float occlusion = 0.0;

    float bias = 0.025;
    for (int i = 0; i < 64; i++)
    {
        vec3 samplePos = TBN * samples[i]; // transform from tangent space to view space
        samplePos = fragPosView + samplePos * radius; 

        vec4 offset = vec4(samplePos, 1.0);
        offset = proj * offset; // view space to clip space
        offset.xyz /= offset.w; // perspective divide to convert to NDC (-1.0 - 1.0)
        offset.xyz  = offset.xyz * 0.5 + 0.5; // transform to range (0.0 - 1.0)  

        float sampleDepth = -depth;

        float isOccluded = (sampleDepth >= samplePos.z + bias) ? 1.0 : 0.0;

        float rangeCheck = smoothstep(0.0, 1.0, radius / abs(fragPosView.z - sampleDepth));

        occlusion += isOccluded * rangeCheck;
    }

    // normalize and invert so 1.0 is lit, 0.0 is shadowed
    occlusion = 1.0 - (occlusion / 64.0);
    
    FragColor = occlusion;
}