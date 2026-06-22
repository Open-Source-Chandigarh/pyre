// lightingCommon.glsl
// assumes GetDiffuseColor(), GetSpecularColor(), GetShininess() are available.
#include "globalUbos.glsl"
// we use a texture array because we have multiple shadow maps (one for each cascade)
uniform sampler2DArray shadowMap;
uniform samplerCubeArrayShadow pointShadowMap;
uniform samplerCube irradianceMap;
uniform int hasIrradianceMap;

uniform samplerCube prefilterMap;
uniform sampler2D brdfLUT;

uniform float iblStrength;

// Poisson Disk Sample Pattern (Standard 20 samples)
vec3 sampleOffsetDirections[20] = vec3[]
(
   vec3( 1,  1,  1), vec3( 1, -1,  1), vec3(-1, -1,  1), vec3(-1,  1,  1), 
   vec3( 1,  1, -1), vec3( 1, -1, -1), vec3(-1, -1, -1), vec3(-1,  1, -1),
   vec3( 1,  1,  0), vec3( 1, -1,  0), vec3(-1, -1,  0), vec3(-1,  1,  0),
   vec3( 1,  0,  1), vec3(-1,  0,  1), vec3( 1,  0, -1), vec3(-1,  0, -1),
   vec3( 0,  1,  1), vec3( 0, -1,  1), vec3( 0, -1, -1), vec3( 0,  1, -1)
);

const float PI = 3.14159265359;

// Normal Distribution Function (Trowbridge-Reitz GGX)
// a^2 / PI * ((n dot h)^2 (a^2 - 1) + 1)^2
float DistributionGGX(vec3 N, vec3 H, float roughness) 
{
    roughness = max(roughness, 0.04);
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom + 0.00001;
    
    return num / denom;
}

// Geometry Function (Schlick-GGX)
// n dot v / n dot v * (1 - k) + k
// k = (roughness + 1) ^ 2 / 8
float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0; // k for direct lighting

    float num = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return num / denom;
}

// Geomtery function (Smith)
// to acccount for both shadowing and masking
// GeometryGGX(NdotV, k) *  GeometryGGX(NdotL, k) V is camera direction and L is light direction
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) 
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness); // view masking
    float ggx1 = GeometrySchlickGGX(NdotL, roughness); // light shadowing
    
    return ggx1 * ggx2;
}

// Fresnel Equation (Fresnel-Schlick Approximation)
vec3 FresnelSchlick(float cosTheta, vec3 F0) 
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// Sébastien Lagarde's Roughness Fresnel Hack for Ambient Light
vec3 FresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0));
}

float CalcPointShadow(int lightIndex, vec3 fragPos, vec3 lightPos, vec3 normal)
{
    vec3 fragToLight = fragPos - lightPos;
    float currentDepth = length(fragToLight);
    
    // dynamic bias based on surface angle to prevent shadow acne
    vec3 lightDir = normalize(lightPos - fragPos);
    float bias = max(0.5 * (1.0 - dot(normal, lightDir)), 0.1);

    float lightRadius = point_params[lightIndex].w; 
    
    // Map the depth to [0, 1] range just like we did when rendering the shadow map
    float normalizedDepth = (currentDepth - bias) / lightRadius;
    
    // If we are beyond the light's range, there is no shadow
    if(normalizedDepth > 1.0) return 0.0;

    float shadow = 0.0;
    int samples = 20;
    
    // For "Real" looking shadows, the blur radius should be small and tight.
    // If this is too large, we get "Peter Panning"
    float diskRadius = 0.01; 

    for(int i = 0; i < samples; ++i)
    {
        // samplerCubeArrayShadow takes 3 parameters:
        // 1. The Sampler
        // 2. A vec4(Direction.x, Direction.y, Direction.z, LayerIndex)
        // 3. The Depth Value to compare against (normalizedDepth)
        
        vec3 sampleDir = fragToLight + sampleOffsetDirections[i] * diskRadius;
        
        // The hardware does the heavy lifting here. It checks neighbors and smooths the result.
        // Returns 1.0 if LIT, 0.0 if SHADOWED (or value in between)
        shadow += texture(pointShadowMap, vec4(sampleDir, lightIndex), normalizedDepth);
    }
    
    shadow /= float(samples);
    
    // Invert result: 1.0 = Shadow, 0.0 = Light
    // texture() returns "Visibility" (1.0 = Visible/Lit)
    return 1.0 - shadow;
}

// this function calculates how much a pixel is in shadow
// 1.0 means fully in shadow, 0.0 means fully lit
float ShadowCalculation(vec3 fragPosWorldSpace, vec3 normal, vec3 lightDir)
{
    // step 1: pick the right cascade layer
    // we need to know how far this pixel is from the camera
    vec4 fragPosViewSpace = view * vec4(fragPosWorldSpace, 1.0);
    float depthValue = abs(fragPosViewSpace.z);

    int layer = -1;
    // since we packed the distances, accessing them dynamically 
    // with a variable index like [i] is hard for the compiler.
    for (int i = 0; i < cascadeCount; ++i)
    {
        // unpack the value manually from the vec4 array
        int vecIdx = i / 4;
        int compIdx = i % 4;
        float splitDist = cascadePlaneDistances[vecIdx][compIdx];

        if (depthValue < splitDist)
        {
            layer = i;
            break;
        }
    }
    
    // if the pixel is very far away (past the last split), use the furthest cascade
    if (layer == -1)
    {
        layer = cascadeCount;
    }

    // step 2: transform to light space
    // now that we know which box/layer we are in, use that specific matrix
    vec4 fragPosLightSpace = lightSpaceMatrices[layer] * vec4(fragPosWorldSpace, 1.0);

    // step 3: manual perspective divide
    // this turns the coordinates into -1 to 1 range
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    
    // transform to 0 to 1 range so we can sample the texture
    projCoords = projCoords * 0.5 + 0.5;

    // step 4: calculate bias
    // since we are now using glPolygonOffset in C++, the hardware handles slope bias dynamically
    // we only need a tiny bias to prevent minor floating point inaccuracies
    float bias = max(0.005 * (1.0 - dot(normal, lightDir)), 0.001);

    // step 5: pcf (soft shadows)
    // instead of testing just one pixel, we test the surrounding pixels and average them
    float shadow = 0.0;
    vec2 texelSize = 1.0 / vec2(textureSize(shadowMap, 0));
    float currentDepth = projCoords.z;

    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
            // we sample the depth map
            // notice the .z component in the vector is the layer index we selected earlier
            float pcfDepth = texture(
                                shadowMap, 
                                vec3(projCoords.xy + vec2(x, y) * texelSize, layer)
                             ).r; 
            
            // if the pixel is deeper than the shadow map value, it is in shadow
            shadow += (currentDepth - bias) > pcfDepth ? 1.0 : 0.0;        
        }    
    }
    // average the 9 samples
    shadow /= 9.0;

    // fix for edge cases
    // if the pixel is outside the light's view distance, force it to be lit
    if(projCoords.z > 1.0)
    {
        shadow = 0.0;
    }

    return shadow;
}

vec3 CalcDirLight(vec3 normal, vec3 fragPos, vec3 viewDir, vec2 uv, vec3 albedo, float roughness, float metallic, float ssao)
{
    vec3 L = normalize((-vec3(dir_direction)));
    vec3 H = normalize(viewDir + L);

    // radiance for a directional light is constant (no attenuation)
    vec3 radiance = vec3(dir_diffuse);

    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);

    float NDF = DistributionGGX(normal, H, roughness);
    float G = GeometrySmith(normal, viewDir, L, roughness);
    vec3 F = FresnelSchlick(max(dot(H, viewDir), 0.0), F0);

    vec3 kS = F;
    vec3 kD = vec3(1.0) - F;
    kD *= 1.0 - metallic;

    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(normal, viewDir), 0.05) * max(dot(normal, L), 0.05) + 0.0001;
    vec3 specular = numerator / denominator;

    float NdotL = max(dot(normal, L), 0.0);
    vec3 Lo = (kD * albedo / PI + specular) * radiance * NdotL;

    vec3 ambient;
    if (hasIrradianceMap == 1)
    {
        vec3 kSAmbient = FresnelSchlickRoughness(max(dot(normal, viewDir), 0.0), F0, roughness);

        vec3 kDAmbient = 1.0 - kSAmbient;
        kDAmbient *= 1.0 - metallic; // pure metals have no diffuse light

        vec3 irradiance = texture(irradianceMap, normal).rgb * iblStrength;
        vec3 diffuseAmbient = irradiance * albedo;

        vec3 R = reflect(-viewDir, normal);

        // sample the prefilter map, we use textureLod to pick the correct blur level based on roughness
        // we baked 5 mip levels (0 to 4), so max LOD is 4.0
        const float MAX_REFLECTION_LOD = 4.0;
        vec3 prefilteredColor = textureLod(prefilterMap, R, roughness * MAX_REFLECTION_LOD).rgb * iblStrength;

        // sample our BRDF LUT
        vec2 envBRDF = texture(brdfLUT, vec2(max(dot(normal, viewDir), 0.0), roughness)).rg;

        // combine prefilteredColor and envBRDF
        vec3 specularAmbient = prefilteredColor * (F0 * envBRDF.x + envBRDF.y);

        // combine diffuseAmbient with specularAmbient and ssao
        ambient = (kDAmbient * diffuseAmbient + specularAmbient) * ssao;
    }
    else 
    {
        ambient = vec3(dir_ambient) * albedo * ssao;
    }
    float shadow = ShadowCalculation(fragPos, normal, L);

    return ambient + (1.0 - shadow) * Lo;
}

vec3 CalcPointLightBlinnPhong(int idx, vec3 normal, vec3 fragPos, vec3 viewDir, vec2 uv)
{
    // standard lighting logic
    vec3 lightPos = vec3(point_position[idx]);
    vec3 lightDir = normalize(lightPos - fragPos);
    
    float diff = max(dot(normal, lightDir), 0.0);
    
    vec3 halfWayDir = normalize(lightDir + viewDir);
    float shininess = GetShininess();
    float spec = pow(max(dot(normal, halfWayDir), 0.0), shininess);

    float constant = point_params[idx].x;
    float linear   = point_params[idx].y;
    float quadratic= point_params[idx].z;
    
    float distance = length(lightPos - fragPos);
    float attenuation = 1.0 / (constant + linear * distance + quadratic * distance * distance);

    vec3 diffuseColor = GetDiffuseColor(uv);
    vec3 specColor = GetSpecularColor(uv);

    vec3 ambient  = vec3(point_ambient[idx]) * diffuseColor;
    vec3 diffuse  = vec3(point_diffuse[idx]) * diff * diffuseColor;
    vec3 specular = vec3(point_specular[idx]) * spec * specColor;

    ambient  *= attenuation;
    diffuse  *= attenuation;
    specular *= attenuation;

    float shadow = CalcPointShadow(idx, fragPos, vec3(point_position[idx]), normal);

    return ambient + (1.0 - shadow) * (diffuse + specular);
}

vec3 CalcPointLight(int idx, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 albedo, float roughness, float metallic)
{
    vec3 lightPos = vec3(point_position[idx]);
    vec3 L = normalize(lightPos - fragPos);
    vec3 H = normalize(viewDir + L); // halway vector

    float distance = length(lightPos - fragPos);
    float radius = point_params[idx].w;
    float physicalAttenuation = 1.0 / max(distance * distance, 0.001);

    // windowing function from ue4 to to smoothly scale light to 0.0 when at the edge of the radius
    float distanceRatio = distance / radius;
    float distanceRatio4 = distanceRatio * distanceRatio * distanceRatio * distanceRatio;
    float falloffWindow = clamp(1.0 - distanceRatio4, 0.0, 1.0);
    falloffWindow = falloffWindow * falloffWindow;

    float attenuation = physicalAttenuation * falloffWindow;

    if (attenuation <= 0.0) return vec3(0.0);

    vec3 radiance = vec3(point_diffuse[idx]) * attenuation; // incoming radiance (irradiance)

    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);

    float NDF = DistributionGGX(normal, H, roughness);
    float G = GeometrySmith(normal, viewDir, L, roughness);
    vec3 F = FresnelSchlick(max(dot(H, viewDir), 0.0), F0);

    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;

    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(normal, viewDir), 0.05) * max(dot(normal, L), 0.05) + 0.0001;
    vec3 specular = numerator / denominator;

    // final outgoing radiance (Lo)
    float NdotL = max(dot(normal, L), 0.0);                
    vec3 Lo = (kD * albedo / PI + specular) * radiance * NdotL;

    float shadow = CalcPointShadow(idx, fragPos, lightPos, normal);

    return (1.0 - shadow) * Lo;
}

vec3 CalcSpotLight(int idx, vec3 normal, vec3 fragPos, vec3 viewDir, vec2 uv)
{
    // standard lighting logic, no changes for csm here
    vec3 lightPos = vec3(spot_position[idx]);
    vec3 lightDir = normalize(lightPos - fragPos);
    
    float diff = max(dot(normal, lightDir), 0.0);
    
    vec3 halfWayDir = normalize(lightDir + viewDir);
    float shininess = GetShininess();
    float spec = pow(max(dot(normal, halfWayDir), 0.0), shininess);

    float distance = length(lightPos - fragPos);
    float constant = spot_params[idx].x;
    float linear   = spot_params[idx].y;
    float quadratic= spot_params[idx].z;
    float attenuation = 1.0 / (constant + linear * distance + quadratic * distance * distance);

    float theta = dot(lightDir, normalize(-vec3(spot_direction[idx])));
    float inner = spot_cutoffs[idx].x;
    float outer = spot_cutoffs[idx].y;
    float epsilon = inner - outer;
    float intensity = clamp((theta - outer) / epsilon, 0.0, 1.0);

    vec3 diffuseColor = GetDiffuseColor(uv);
    vec3 specColor = GetSpecularColor(uv);

    vec3 ambient  = vec3(spot_ambient[idx]) * diffuseColor;
    vec3 diffuse  = vec3(spot_diffuse[idx]) * diff * diffuseColor;
    vec3 specular = vec3(spot_specular[idx]) * spec * specColor;

    diffuse  *= intensity;
    specular *= intensity;
    
    ambient  *= attenuation;
    diffuse  *= attenuation;
    specular *= attenuation;

    return ambient + diffuse + specular;
}