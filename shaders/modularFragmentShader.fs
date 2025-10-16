#version 330 core

out vec4 FragColor;

in vec3 Normal;  
in vec3 FragPos;  
in vec2 TexCoords;

struct Material {
    sampler2D diffuse;
    sampler2D specular;
    vec3 diffuseColor;
    vec3 specularColor;
    float shininess;
    bool useDiffuseMap;
    bool useSpecularMap;
};

struct DirLight {
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct PointLight {
    vec3 position;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float constant;
    float linear;
    float quadratic;
};

struct SpotLight {
    vec3 position;
    vec3 direction;
    float innerCutOff;
    float outerCutOff;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float constant;
    float linear;
    float quadratic;
};

#define MAX_POINT_LIGHTS 8
#define MAX_SPOT_LIGHTS 4

uniform int numPointLights;
uniform int numSpotLights;

uniform DirLight dirLight;
uniform PointLight pointLights[MAX_POINT_LIGHTS];
uniform SpotLight spotLights[MAX_SPOT_LIGHTS];
uniform Material material;
uniform vec3 viewPos;

// Function declarations
vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir);
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir);
vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir);

void main()
{
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);

    // Combine lighting contributions
    vec3 result = CalcDirLight(dirLight, norm, viewDir);
    for (int i = 0; i < numPointLights; i++)
        result += CalcPointLight(pointLights[i], norm, FragPos, viewDir);
    for (int i = 0; i < numSpotLights; i++)
        result += CalcSpotLight(spotLights[i], norm, FragPos, viewDir);

    FragColor = vec4(result, 1.0);
}

// Helper to get material colors
vec3 GetDiffuseColor()
{
    if (material.useDiffuseMap)
        return texture(material.diffuse, TexCoords).rgb;
    else
        return material.diffuseColor;
}

vec3 GetSpecularColor()
{
    if (material.useSpecularMap)
        return texture(material.specular, TexCoords).rgb;
    else
        return material.specularColor;
}

// ------------------- DIRECTIONAL LIGHT -------------------
vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir)
{
    vec3 lightDir = normalize(-light.direction);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);

    vec3 ambient  = light.ambient  * GetDiffuseColor();
    vec3 diffuse  = light.diffuse  * diff * GetDiffuseColor();
    vec3 specular = light.specular * spec * GetSpecularColor();

    return ambient + diffuse + specular;
}

// ------------------- POINT LIGHT -------------------
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 lightDir = normalize(light.position - fragPos);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);

    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + 
                               light.quadratic * (distance * distance));

    vec3 ambient  = light.ambient  * GetDiffuseColor();
    vec3 diffuse  = light.diffuse  * diff * GetDiffuseColor();
    vec3 specular = light.specular * spec * GetSpecularColor();

    ambient  *= attenuation;
    diffuse  *= attenuation;
    specular *= attenuation;

    return ambient + diffuse + specular;
}

// ------------------- SPOT LIGHT -------------------
vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 lightDir = normalize(light.position - fragPos);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);

    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance +
                               light.quadratic * (distance * distance));

    float theta = dot(lightDir, normalize(-light.direction));
    float epsilon = light.innerCutOff - light.outerCutOff;
    float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);

    vec3 ambient  = light.ambient  * GetDiffuseColor();
    vec3 diffuse  = light.diffuse  * diff * GetDiffuseColor();
    vec3 specular = light.specular * spec * GetSpecularColor();

    diffuse  *= intensity;
    specular *= intensity;
    ambient  *= attenuation;
    diffuse  *= attenuation;
    specular *= attenuation;

    return ambient + diffuse + specular;
}