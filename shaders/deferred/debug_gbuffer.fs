#version 420 core
out vec4 FragColor;

in vec2 TexCoords;

layout (binding = 5) uniform sampler2D gPosition;
layout (binding = 6) uniform sampler2D gNormal;
layout (binding = 7) uniform sampler2D gAlbedoSpec;

uniform int displayMode; 

void main()
{
    vec4 posData = texture(gPosition, TexCoords);
    vec4 normData = texture(gNormal, TexCoords);
    vec4 albedoData = texture(gAlbedoSpec, TexCoords);

    // World Position
    if (displayMode == 1) 
    {
        FragColor = vec4(posData.rgb, 1.0);
    }
    // Normals
    else if (displayMode == 2) 
    {
        vec3 n = normData.rgb;
        FragColor = vec4(n * 0.5 + 0.5, 1.0); 
    }
    // Albedo (Base Color)
    else if (displayMode == 3) 
    {
        FragColor = vec4(albedoData.rgb, 1.0);
    }
    // Roughness (Stored in Normal Alpha)
    else if (displayMode == 4) 
    {
        float roughness = normData.a;
        FragColor = vec4(vec3(roughness), 1.0); // Grayscale
    }
    // Metallic / Specular Intensity (Stored in Albedo Alpha)
    else if (displayMode == 5) 
    {
        float metal = albedoData.a;
        FragColor = vec4(vec3(metal), 1.0); // Grayscale
    }
    // Linear Depth (Stored in Position Alpha)
    else if (displayMode == 6)
    {
        float depth = posData.a;
        // Divide by Z-Far (approx 100.0) to visualize
        FragColor = vec4(vec3(depth / 100.0), 1.0); 
    }
    else 
    {
        FragColor = vec4(1.0, 0.0, 1.0, 1.0);
    }
}