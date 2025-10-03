#version 330 core
struct Material
{
    sampler2D diffuse;
    sampler2D specular;
    float shininess;
};

struct Light
{
    float cutOff;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;

    float constant;
    float linear;
    float quadratic;
};

out vec4 FragColor;

in vec3 LightPos;
in vec3 LightDirection; 
in vec3 Normal;  
in vec3 FragPos;  
in vec2 TexCoords;

uniform Material material;
uniform Light light;

void main()
{
    
    vec3 lightDir = normalize(LightPos - FragPos);

    float theta = dot(lightDir, normalize(-LightDirection));


    if(theta > light.cutOff)
    {
        vec3 ambient = vec3(texture(material.diffuse, TexCoords)) * light.ambient; 
        // diffuse 
        vec3 norm = normalize(Normal);
        float diff = max(dot(norm, lightDir), 0.0);
        vec3 diffuse = diff * light.diffuse * vec3(texture(material.diffuse, TexCoords));
    
        // specular
        vec3 viewDir = normalize(-FragPos); // the viewer is always at (0,0,0) in view-space, so viewDir is (0,0,0) - Position => -Position
        vec3 reflectDir = reflect(-lightDir, norm);  
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
        vec3 specular = vec3(texture(material.specular, TexCoords)) * spec * light.specular; 

        // attenuation
        float distance    = length(LightPos - FragPos);
        float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));    

        // ambient  *= attenuation; // remove attenuation from ambient, as otherwise at large distances the light would be darker inside than outside the spotlight due the ambient term in the else branch
        diffuse   *= attenuation;
        specular *= attenuation;   
        vec3 result = (ambient + diffuse + specular);

        FragColor = vec4(result, 1.0);
    }
    else
    {
        // else, use ambient light so scene isn't completely dark outside the spotlight.
        FragColor = vec4(light.ambient * texture(material.diffuse, TexCoords).rgb, 1.0);
    }
   
} 