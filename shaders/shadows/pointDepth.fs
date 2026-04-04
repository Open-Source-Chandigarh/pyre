#version 420 core
in vec4 FragPos;
#include "../includes/globalUbos.glsl"

void main()
{
    // get distance between fragment and light source
    float lightDistance = length(FragPos.xyz - lightPos.xyz);
    
    // map to [0-1] range by dividing by far_plane
    lightDistance = lightDistance / farPlane;
    
    // wite this as modified depth
    gl_FragDepth = lightDistance;
}