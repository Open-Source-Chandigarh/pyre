// skybox.fs
#version 420 core
out vec4 FragColor;
out vec4 BrightColor;

in vec3 TexCoords;

uniform samplerCube material_skybox;

void main()
{    
    FragColor = texture(material_skybox, TexCoords);
    BrightColor = vec4(0.0, 0.0, 0.0, 1.0);
}