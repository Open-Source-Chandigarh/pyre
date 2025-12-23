// lightingCommon.glsl
// Assumes GetDiffuseColor(), GetSpecularColor(), GetShininess() are available.

uniform sampler2D shadowMap;

float ShadowCalculation(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir)
{
    // manual perspective divide 
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    // transform to [0,1] range so we can sample the depth map.
    projCoords = projCoords * 0.5 + 0.5; 

    if(projCoords.z > 1.0)
        return 0.0;
    // projCoords directly correspond to the transformed NDC coordinates from the first render pass.
    float closestDepth = texture(shadowMap, projCoords.xy).r;  
    // current depth at this fragment is the projected vector's z coordinate 
    float currentDepth = projCoords.z;  
    // add bias to prevent shadow acne, bias is based on the angle between the surface normal and lightdir which we get through dot 
    float bias = max(0.004 * (1.0 - dot(normal, lightDir)), 0.0004);  // light directly above angle 90 cos90 is 0 so 0.05 is applied 
    // pcf (percentage closer filterting)
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r; 
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;        
        }    
    }
    shadow /= 9.0;
    return shadow;
}

vec3 CalcDirLight(vec3 normal, vec3 fragPos, vec3 viewDir, vec4 fragPosLightSpace)
{
    vec3 lightDir = normalize(-vec3(dir_direction));
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 halfWayDir = normalize(lightDir + viewDir);

    float shininess = GetShininess();
    float spec = pow(max(dot(normal, halfWayDir), 0.0), shininess);

    vec3 diffuseColor = GetDiffuseColor();
    vec3 specColor = GetSpecularColor();

    vec3 ambient  = vec3(dir_ambient) * diffuseColor;
    vec3 diffuse  = vec3(dir_diffuse) * diff * diffuseColor;
    vec3 specular = vec3(dir_specular) * spec * specColor;
    float shadow = ShadowCalculation(fragPosLightSpace, normal, lightDir);     

    return (ambient + (1.0 - shadow) * (diffuse + specular));
}

vec3 CalcPointLight(int idx, vec3 normal, vec3 fragPos, vec3 viewDir)
{
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