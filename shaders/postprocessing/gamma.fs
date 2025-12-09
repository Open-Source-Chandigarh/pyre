#version 330 core
out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D scene;
uniform float gamma;

void main()
{
    vec4 texColor = texture(scene, TexCoords);
    vec3 corrected = pow(texColor.rgb, vec3(1.0 / gamma));
    
    FragColor = vec4(corrected, texColor.a);
}