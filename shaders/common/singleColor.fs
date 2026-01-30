// singleColor.fs
#version 330 core

out vec4 FragColor;
out vec4 BrightColor;

uniform vec3 color;
uniform float bloomFactor;

void main()
{
	FragColor = vec4(color, 1.0);
	float brightness = dot(FragColor.rgb, vec3(0.2126, 0.7152, 0.0722));
	if((brightness * bloomFactor) > 1.0) 
        BrightColor = vec4(FragColor.rgb, 1.0);
    else
        BrightColor = vec4(0.0, 0.0, 0.0, 1.0);
}