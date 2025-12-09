// skybox.fs
#version 420 core
out vec4 FragColor;

in vec3 TexCoords;

uniform samplerCube material_skybox;

void main()
{    
    FragColor = texture(material_skybox, TexCoords);
}