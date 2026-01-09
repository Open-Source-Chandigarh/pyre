#version 420 core

// input:
// we work on triangles.
// 'invocations = 4' tells the gpu to run this code 4 times at once for every triangle.
// we need 4 times because we made 4 shadow cascades in c++ (near, mid, far, very far).
layout(triangles, invocations = 4) in;

// output:
// we output a triangle strip. max vertices is 3 because we just clone the input triangle.
layout(triangle_strip, max_vertices = 3) out;

// binding 2: CSM Shadow Data
layout (std140, binding = 2) uniform ShadowData
{
    mat4 lightSpaceMatrices[16];    // 1024 bytes
    vec4 cascadePlaneDistances[4];  // 64 bytes (Stores 16 split distances packed)
    int cascadeCount;               // 4 bytes
    float shadowFarPlane;           // 4 bytes 
    float _padCSM1;                 // 4 bytes
    float _padCSM2;                 // 4 bytes (Align to 16)
};

void main()
{          
    // gl_InvocationID is a number from 0 to 3.
    // it tells us which "clone" we are working on right now.
    // we use it to pick the right matrix and the right texture layer.
    
    for (int i = 0; i < 3; ++i)
    {
        // 1. transform position
        // gl_in[i].gl_Position holds the world position from the vertex shader.
        // we multiply it by the matrix for this specific cascade layer.
        gl_Position = lightSpaceMatrices[gl_InvocationID] * gl_in[i].gl_Position;
        
        // 2. send to texture layer
        // this variable tells the gpu: "draw this triangle onto layer n of the 3d texture."
        gl_Layer = gl_InvocationID;
        
        // send this vertex to the next stage
        EmitVertex();
    }
    
    // we are done with this triangle clone
    EndPrimitive();
}