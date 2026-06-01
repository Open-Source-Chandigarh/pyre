// lightingCommon.glsl
// assumes GetDiffuseColor(), GetSpecularColor(), GetShininess() are available.
#include "globalUbos.glsl"
// we use a texture array because we have multiple shadow maps (one for each cascade)
uniform sampler2DArray shadowMap;

uniform samplerCubeArrayShadow pointShadowMap;
uniform float pointShadowFarPlane;

// Poisson Disk Sample Pattern (Standard 20 samples)
vec3 sampleOffsetDirections[20] = vec3[]
(
   vec3( 1,  1,  1), vec3( 1, -1,  1), vec3(-1, -1,  1), vec3(-1,  1,  1), 
   vec3( 1,  1, -1), vec3( 1, -1, -1), vec3(-1, -1, -1), vec3(-1,  1, -1),
   vec3( 1,  1,  0), vec3( 1, -1,  0), vec3(-1, -1,  0), vec3(-1,  1,  0),
   vec3( 1,  0,  1), vec3(-1,  0,  1), vec3( 1,  0, -1), vec3(-1,  0, -1),
   vec3( 0,  1,  1), vec3( 0, -1,  1), vec3( 0, -1, -1), vec3( 0,  1, -1)
);

float CalcPointShadow(int lightIndex, vec3 fragPos, vec3 lightPos, vec3 normal)
{
    vec3 fragToLight = fragPos - lightPos;
    float currentDepth = length(fragToLight);
    
    // dynamic bias based on surface angle to prevent shadow acne
    vec3 lightDir = normalize(lightPos - fragPos);
    float bias = max(0.5 * (1.0 - dot(normal, lightDir)), 0.1); 
    
    // Map the depth to [0, 1] range just like we did when rendering the shadow map
    float normalizedDepth = (currentDepth - bias) / pointShadowFarPlane;
    
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
    // bias prevents shadow acne (weird lines on objects)
    // we calculate it based on the angle of the light
    float bias = max(0.001 * (1.0 - dot(normal, lightDir)), 0.00025);

    // distant cascades cover huge areas, so one pixel is very large
    // if we use a small bias, shadows will detach from objects (peter panning)
    // so we scale the bias based on how far away the cascade is
    
    const float biasModifier = 0.15;
    
    if (layer == cascadeCount)
    {
        // far plane logic
        bias *= 1.0 / (shadowFarPlane * biasModifier);
        bias *= 10.0; // massive bias for the furthest hills to kill acne
    }
    else
    {
        int vecIdx = layer / 4;
        int compIdx = layer % 4;
        float splitDist = cascadePlaneDistances[vecIdx][compIdx];
        bias *= 1.0 / (splitDist * biasModifier);
        
        // progressively increase bias for further layers
        // layer 0 gets 1.0 (no change), layer 1 gets 4.0, layer 2 gets 8.0, etc.
        bias *= (1.0 + (layer * 4.0));
    }

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

vec3 CalcDirLight(vec3 normal, vec3 fragPos, vec3 viewDir, vec2 uv, float ssao)
{
    vec3 lightDir = normalize(-vec3(dir_direction));
    
    // diffuse shading
    // calculate how much the surface faces the light
    float diff = max(dot(normal, lightDir), 0.0);
    
    // specular shading
    // blinn-phong method using the halfway vector
    vec3 halfWayDir = normalize(lightDir + viewDir);
    float shininess = GetShininess();
    float spec = pow(max(dot(normal, halfWayDir), 0.0), shininess);

    vec3 diffuseColor = GetDiffuseColor(uv);
    vec3 specColor = GetSpecularColor(uv);

    // combine results
    // ambient light is always present
    vec3 ambient  = vec3(dir_ambient) * diffuseColor * ssao;
    vec3 diffuse  = vec3(dir_diffuse) * diff * diffuseColor;
    vec3 specular = vec3(dir_specular) * spec * specColor;
    
    // calculate shadow
    // we pass the world position directly to our helper function
    float shadow = ShadowCalculation(fragPos, normal, lightDir);    

    // apply shadow to diffuse and specular, but never ambient
    return (ambient + (1.0 - shadow) * (diffuse + specular));
}

vec3 CalcPointLight(int idx, vec3 normal, vec3 fragPos, vec3 viewDir, vec2 uv)
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

vec2 ParallaxMapping(vec2 texCoords, vec3 viewDir)
{
    const float minLayers = 8.0;
    const float maxLayers = 32.0;
    float numLayers = mix(maxLayers, minLayers, max(dot(vec3(0.0, 0.0, 1.0), viewDir), 0.0));  
    // calculate the size of each layer
    float layerDepth = 1.0 / numLayers;
    // depth of current layer
    float currentLayerDepth = 0.0;
    // the amount to shift the texture coordinates per layer (from vector P)
    vec2 P = viewDir.xy * 0.03; 
    vec2 deltaTexCoords = P / numLayers;

    // get initial values
    vec2 currentTexCoords = texCoords;
    float currentDepthMapValue = texture(material_displacement, currentTexCoords).r;

    while(currentLayerDepth < currentDepthMapValue)
    {
        // shift texture coordinates along direction of P
        currentTexCoords -= deltaTexCoords;
        // get depthmap value at current texture coordinates
        currentDepthMapValue = texture(material_displacement, currentTexCoords).r;  
        // get depth of next layer
        currentLayerDepth += layerDepth;  
    }

    // get texture coordinates before collision (reverse operations)
    vec2 prevTexCoords = currentTexCoords + deltaTexCoords;

    // get depth after and before collision for linear interpolation
    float afterDepth  = currentDepthMapValue - currentLayerDepth;
    float beforeDepth = texture(material_displacement, prevTexCoords).r - currentLayerDepth + layerDepth;

    // interpolation of texture coordinates
    float weight = afterDepth / (afterDepth - beforeDepth);
    vec2 finalTexCoords = prevTexCoords * weight + currentTexCoords * (1.0 - weight);
    return finalTexCoords;
}