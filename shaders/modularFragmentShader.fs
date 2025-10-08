#version 330 core
out vec4 FragColor;

in vec3 FragPos;   // now world-space
in vec3 Normal;    // now world-space
in vec2 TexCoords;

uniform vec3 viewPos; // camera position in world space

struct Material {
    sampler2D diffuse;
    sampler2D specular;
    vec3 diffuseColor;
    vec3 specularColor;
    float shininess;
    bool useDiffuseTex;
    bool useSpecularTex;
};

struct DirLight {
    vec3 direction; // world-space direction (direction from light towards scene)
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct PointLight {
    vec3 position;  // world-space
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float constant;
    float linear;
    float quadratic;
};

struct SpotLight {
    vec3 position;  // world-space
    vec3 direction; // world-space
    float innerCutOff;
    float outerCutOff;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float constant;
    float linear;
    float quadratic;
};

#define NR_POINT_LIGHTS 4

uniform Material material;
uniform DirLight dirLight;
uniform PointLight pointLights[NR_POINT_LIGHTS];
uniform SpotLight spotLight;

vec3 CalcDirLight(DirLight light, vec3 normal);
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos);
vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos);

void main()
{
    vec3 norm = normalize(Normal);
    vec3 result = vec3(0.0);

    // Directional Light
    result += CalcDirLight(dirLight, norm);

    // Point Lights
    for(int i = 0; i < NR_POINT_LIGHTS; i++)
        result += CalcPointLight(pointLights[i], norm, FragPos);

    // Spot Light
    result += CalcSpotLight(spotLight, norm, FragPos);

    FragColor = vec4(result, 1.0);
}

// ---------- LIGHT CALC FUNCTIONS ----------

vec3 CalcDirLight(DirLight light, vec3 normal)
{
    // If you stored 'direction' as the direction the light points (e.g. (-0.2, -1, -0.3)),
    // then the vector from fragment toward the light is simply -light.direction:
    vec3 lightDir = normalize(-light.direction); // world-space
    vec3 viewDir = normalize(viewPos - FragPos);

    float diff = max(dot(normal, lightDir), 0.0);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);

    vec3 baseDiffuse = material.useDiffuseTex ? vec3(texture(material.diffuse, TexCoords)) : material.diffuseColor;
    vec3 baseSpec = material.useSpecularTex ? vec3(texture(material.specular, TexCoords)) : material.specularColor;

    vec3 ambient = light.ambient * baseDiffuse;
    vec3 diffuse = light.diffuse * diff * baseDiffuse;
    vec3 specular = light.specular * spec * baseSpec;

    return ambient + diffuse + specular;
}

vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos)
{
    vec3 lightDir = normalize(light.position - fragPos); // world-space direction to light
    vec3 viewDir = normalize(viewPos - fragPos);

    float diff = max(dot(normal, lightDir), 0.0);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);

    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * distance * distance);

    vec3 baseDiffuse = material.useDiffuseTex ? vec3(texture(material.diffuse, TexCoords)) : material.diffuseColor;
    vec3 baseSpec = material.useSpecularTex ? vec3(texture(material.specular, TexCoords)) : material.specularColor;

    vec3 ambient = baseDiffuse * light.ambient;
    vec3 diffuse = diff * baseDiffuse * light.diffuse;
    vec3 specular = spec * baseSpec * light.specular;

    return (ambient + diffuse + specular) * attenuation;
}

vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos)
{
    vec3 lightDir = normalize(light.position - fragPos);
    vec3 viewDir = normalize(viewPos - fragPos);
    vec3 reflectDir = reflect(-lightDir, normal);

    float diff = max(dot(normal, lightDir), 0.0);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);

    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * distance * distance);

    // spot cone math: assume inner/outer are given as cos(angle)
    float theta = dot(lightDir, normalize(-light.direction));
    float epsilon = light.innerCutOff - light.outerCutOff;
    float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);

    vec3 baseDiffuse = material.useDiffuseTex ? vec3(texture(material.diffuse, TexCoords)) : material.diffuseColor;
    vec3 baseSpec = material.useSpecularTex ? vec3(texture(material.specular, TexCoords)) : material.specularColor;

    vec3 ambient = baseDiffuse * light.ambient;
    vec3 diffuse = diff * baseDiffuse * light.diffuse * intensity;
    vec3 specular = spec * baseSpec * light.specular * intensity;

    return (ambient + diffuse + specular) * attenuation;
}
