// lightingCommon.glsl
// assumes GetDiffuseColor(), GetSpecularColor(), GetShininess() are available.

// we use a texture array because we have multiple shadow maps (one for each cascade)
uniform sampler2DArray shadowMap;

// these values come from the renderer updates every frame
// they tell us where one shadow cascade ends and the next begins
uniform float cascadePlaneDistances[16];
uniform int cascadeCount;
uniform float farPlane;

// this function calculates how much a pixel is in shadow
// 0.0 means fully in shadow, 1.0 means fully lit
float ShadowCalculation(vec3 fragPosWorldSpace, vec3 normal, vec3 lightDir)
{
    // step 1: pick the right cascade layer
    // we need to know how far this pixel is from the camera
    vec4 fragPosViewSpace = view * vec4(fragPosWorldSpace, 1.0);
    float depthValue = abs(fragPosViewSpace.z);

    int layer = -1;
    for (int i = 0; i < cascadeCount; ++i)
    {
        // check if the pixel is closer than this cascade's split distance
        if (depthValue < cascadePlaneDistances[i])
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
        bias *= 1.0 / (farPlane * biasModifier);
        bias *= 10.0; // massive bias for the furthest hills to kill acne
    }
    else
    {
        bias *= 1.0 / (cascadePlaneDistances[layer] * biasModifier);
        
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

vec3 CalcDirLight(vec3 normal, vec3 fragPos, vec3 viewDir)
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

    vec3 diffuseColor = GetDiffuseColor();
    vec3 specColor = GetSpecularColor();

    // combine results
    // ambient light is always present
    vec3 ambient  = vec3(dir_ambient) * diffuseColor;
    vec3 diffuse  = vec3(dir_diffuse) * diff * diffuseColor;
    vec3 specular = vec3(dir_specular) * spec * specColor;
    
    // calculate shadow
    // we pass the world position directly to our helper function
    float shadow = ShadowCalculation(fragPos, normal, lightDir);    

    // apply shadow to diffuse and specular, but never ambient
    return (ambient + (1.0 - shadow) * (diffuse + specular));
}

vec3 CalcPointLight(int idx, vec3 normal, vec3 fragPos, vec3 viewDir)
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

    vec3 diffuseColor = GetDiffuseColor();
    vec3 specColor = GetSpecularColor();

    vec3 ambient  = vec3(point_ambient[idx]) * diffuseColor;
    vec3 diffuse  = vec3(point_diffuse[idx]) * diff * diffuseColor;
    vec3 specular = vec3(point_specular[idx]) * spec * specColor;

    ambient  *= attenuation;
    diffuse  *= attenuation;
    specular *= attenuation;

    return ambient + diffuse + specular;
}

vec3 CalcSpotLight(int idx, vec3 normal, vec3 fragPos, vec3 viewDir)
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

    vec3 diffuseColor = GetDiffuseColor();
    vec3 specColor = GetSpecularColor();

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