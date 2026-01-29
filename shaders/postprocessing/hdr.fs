#version 420 core
out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D scene;

uniform float exposure;

void main()
{
    vec3 hdrColor = texture(scene, TexCoords).rgb;
    // reinhard tone mapping
    vec3 mapped = vec3(1.0) - exp(-hdrColor * exposure);
    FragColor = vec4(mapped, 1.0);
}