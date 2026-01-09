#version 420
#include "../includes/globalUbos.glsl"
layout (triangles) in;
layout (triangle_strip, max_vertices=18) out;

uniform int lightIndex;

out vec4 FragPos; // FragPos from GS (output per emitvertex)

void main()
{
    for(int face = 0; face < 6; face++)
    {
        // gl_Layer logic for Cubemap Array:
        // Layer index = (LightIndex * 6 faces) + CurrentFace
        gl_Layer = (lightIndex * 6) + face;
        gl_Layer = (lightIndex * 6) + face;

        for(int i = 0; i < 3; ++i) // for each triangle vertex
        {
            FragPos = gl_in[i].gl_Position;
            gl_Position = shadowMatrices[face] * FragPos;
            EmitVertex();
        }    
        EndPrimitive();
    }
}