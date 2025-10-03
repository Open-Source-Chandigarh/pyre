#version 330 core
struct Material
{
    sampler2D diffuse;
    sampler2D specular;
    float shininess;
};

struct Light
{
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

out vec4 FragColor;

in vec3 LightPos;
in vec3 LightDirection; // for directional lights
in vec3 Normal;  
in vec3 FragPos;  
in vec2 TexCoords;

uniform Material material;
uniform Light light;

void main()
{
    // ambient
    vec3 ambient = vec3(texture(material.diffuse, TexCoords)) * light.ambient;    
    vec3 lightDir = normalize(-LightDirection);
     // diffuse 
    vec3 norm = normalize(Normal);
    // vec3 lightDir = normalize(LightPos - FragPos); for non directional lights
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * light.diffuse * vec3(texture(material.diffuse, TexCoords));
    
    // specular
    vec3 viewDir = normalize(-FragPos); // the viewer is always at (0,0,0) in view-space, so viewDir is (0,0,0) - Position => -Position
    vec3 reflectDir = reflect(-lightDir, norm);  
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    vec3 specular = vec3(texture(material.specular, TexCoords)) * spec * light.specular; 
    
    vec3 result = (ambient + diffuse + specular);
    FragColor = vec4(result, 1.0);
} 