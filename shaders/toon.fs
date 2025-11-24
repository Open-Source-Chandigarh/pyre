#version 420 core

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

out vec4 FragColor;

// 1. INCLUDE THE SAME SHARED DATA
#include "includes/globalUbos.glsl"
#include "includes/materialCommon.glsl"

// --- HELPER: Toon Stepping Logic ---
// Convert a smooth 0.0-1.0 gradient into hard steps (0.1, 0.5, 1.0)
float Toonify(float value)
{
    if (value > 0.95) return 1.0;
    if (value > 0.5)  return 0.7;
    if (value > 0.25) return 0.35;
    return 0.1; // Shadow color
}

// --- HELPER: Hard Specular ---
// Toon specular is usually a dot, not a fade
float ToonSpecular(vec3 normal, vec3 lightDir, vec3 viewDir)
{
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material_shininess); // Use material shininess
    
    // Hard cutoff for the "glossy dot" look
    return (spec > 0.5) ? 1.0 : 0.0; 
}

// --------------------------------------------------------
// 1. DIRECTIONAL LIGHT (Toon)
// --------------------------------------------------------
vec3 CalcToonDirectional(vec3 normal, vec3 viewDir)
{
    vec3 lightDir = normalize(-vec3(dir_direction));
    
    // Diffuse
    float NdotL = max(dot(normal, lightDir), 0.0);
    float intensity = Toonify(NdotL);
    
    // Specular
    float spec = ToonSpecular(normal, lightDir, viewDir);
    
    // Combine
    vec3 color = GetDiffuseColor() * vec3(dir_diffuse) * intensity;
    color += GetSpecularColor() * vec3(dir_specular) * spec;
    
    return color;
}

// --------------------------------------------------------
// 2. POINT LIGHT (Toon)
// --------------------------------------------------------
vec3 CalcToonPoint(int idx, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 lightPos = vec3(point_position[idx]);
    vec3 lightDir = normalize(lightPos - fragPos);
    
    // Attenuation (Same physics as realistic shader!)
    float distance = length(lightPos - fragPos);
    float constant = point_params[idx].x;
    float linear   = point_params[idx].y;
    float quadratic= point_params[idx].z;
    float attenuation = 1.0 / (constant + linear * distance + quadratic * distance * distance);

    // Diffuse
    float NdotL = max(dot(normal, lightDir), 0.0);
    float intensity = Toonify(NdotL);
    
    // Specular
    float spec = ToonSpecular(normal, lightDir, viewDir);

    // Combine
    vec3 color = GetDiffuseColor() * vec3(point_diffuse[idx]) * intensity;
    color += GetSpecularColor() * vec3(point_specular[idx]) * spec;

    return color * attenuation;
}

// --------------------------------------------------------
// 3. SPOT LIGHT (Toon)
// --------------------------------------------------------
vec3 CalcToonSpot(int idx, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 lightPos = vec3(spot_position[idx]);
    vec3 lightDir = normalize(lightPos - fragPos);
    
    // Attenuation
    float distance = length(lightPos - fragPos);
    float constant = spot_params[idx].x;
    float linear   = spot_params[idx].y;
    float quadratic= spot_params[idx].z;
    float attenuation = 1.0 / (constant + linear * distance + quadratic * distance * distance);

    // Spot Intensity (Cone)
    float theta = dot(lightDir, normalize(-vec3(spot_direction[idx])));
    float inner = spot_cutoffs[idx].x;
    float outer = spot_cutoffs[idx].y;
    float epsilon = inner - outer;
    
    // Standard Soft Edge:
    // float spotIntensity = clamp((theta - outer) / epsilon, 0.0, 1.0);
    
    // TOON HARD EDGE: If inside the cone, full brightness. If outside, dark.
    float spotIntensity = (theta > outer) ? 1.0 : 0.0; 

    // Diffuse
    float NdotL = max(dot(normal, lightDir), 0.0);
    float intensity = Toonify(NdotL);
    
    // Specular
    float spec = ToonSpecular(normal, lightDir, viewDir);

    vec3 color = GetDiffuseColor() * vec3(spot_diffuse[idx]) * intensity;
    color += GetSpecularColor() * vec3(spot_specular[idx]) * spec;

    return color * attenuation * spotIntensity;
}

void main()
{
    vec3 N = normalize(Normal);
    vec3 V = normalize(vec3(viewPos) - FragPos);

    vec3 result = vec3(0.0);

    // 1. Ambient (Base)
    result += vec3(dir_ambient) * GetDiffuseColor();

    // 2. Directional
    result += CalcToonDirectional(N, V);

    // 3. Point Lights (Loop limits from UBO)
    for(int i = 0; i < numPointLights; i++)
    {
        result += CalcToonPoint(i, N, FragPos, V);
    }

    // 4. Spot Lights
    for(int i = 0; i < numSpotLights; i++)
    {
        result += CalcToonSpot(i, N, FragPos, V);
    }

    // 5. Rim Lighting (Optional for extra style)
    float rim = 1.0 - max(dot(V, N), 0.0);
    rim = smoothstep(0.6, 0.7, rim); // Sharp rim
    vec3 rimColor = vec3(1.0) * rim * 0.3;
    result += rimColor;

    FragColor = vec4(result, 1.0);
}