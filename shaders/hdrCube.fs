#version 420 core

out vec4 FragColor;
in vec3 localPos;

uniform sampler2D material_diffuse; // hdr sky map

const vec2 invAtan = vec2(0.1591, 0.3183); // to normalize spherical angles we use inverse of 2pi and pi
vec2 SampleSphericalMap(vec3 v)
{
    // atan gives us the yaw in radians (-pi, pi) and asin gives us the pitch in radians (-pi/2, pi/2)
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y)); 
    uv *= invAtan; // normalize the radian angles
    uv += 0.5; // convert from -0.5, 0.5 to 0.0 to 1.0
    return uv;
}

void main()
{
    vec2 uv = SampleSphericalMap(normalize(localPos));
    vec3 color = texture(material_diffuse, uv).rgb;

    FragColor = vec4(color, 1.0);
}