#version 330 core
out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D scene;

void main()
{
   FragColor = texture(scene, TexCoords);
   float average = 0.2126 * FragColor.r + 0.7152 * FragColor.g + 0.0722 * FragColor.b;
   FragColor = vec4(average, average, average, 1.0);
}