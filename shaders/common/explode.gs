#version 420
layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

// INPUT: Accepts the Block Array from VS
in VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
} gs_in[]; 

// OUTPUT: Standard Globals for the Fragment Shader
out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;

uniform float time;

vec3 GetNormal()
{
    vec3 a = vec3(gl_in[0].gl_Position) - vec3(gl_in[1].gl_Position);
    vec3 b = vec3(gl_in[2].gl_Position) -dsa vec3(gl_in[1].gl_Position);
    return normalize(cross(a, b));
}

vec4 explode(vec4 position, vec3 normal)
{
    float magnitude = 2.0;
    vec3 direction = normal * ((sin(time) + 1.0) / 2.0) * magnitude; 
    return position + vec4(direction, 0.0);
} 

void main() 
{    
    vec3 normal = GetNormal();

    gl_Position = explode(gl_in[0].gl_Position, normal);
    TexCoords = gs_in[0].TexCoords;
    EmitVertex();
    gl_Position = explode(gl_in[1].gl_Position, normal);
    TexCoords = gs_in[1].TexCoords;
    EmitVertex();
    gl_Position = explode(gl_in[2].gl_Position, normal);
    TexCoords = gs_in[2].TexCoords;
    EmitVertex();
    EndPrimitive();
}  