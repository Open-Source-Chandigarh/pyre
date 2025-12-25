// binding 0: camera data (updates every frame)
layout(std140, binding = 0) uniform CameraData
{
    mat4 view;
    mat4 proj;
    vec3 viewPos;
    float padding;
};

// binding 1: lighting data (updates when lights change)
layout(std140, binding = 1) uniform LightsData
{
    // directional light
    vec4 dir_direction;
    vec4 dir_ambient;
    vec4 dir_diffuse;
    vec4 dir_specular;

    // counts
    int numPointLights;
    int numSpotLights;
    int _pad0;
    int _pad1;

    // point lights (max = 8)
    vec4 point_position[8];
    vec4 point_ambient[8];
    vec4 point_diffuse[8];
    vec4 point_specular[8];
    vec4 point_params[8]; 

    // spot lights (max = 4)
    vec4 spot_position[4];
    vec4 spot_direction[4];
    vec4 spot_cutoffs[4]; 
    vec4 spot_ambient[4];
    vec4 spot_diffuse[4];
    vec4 spot_specular[4];
    vec4 spot_params[4];
};

// binding 2: shadow matrices (csm)
// this lets the lighting shader access the matrices to find where
// a pixel lands in the shadow map
layout (std140, binding = 2) uniform ShadowData
{
    mat4 lightSpaceMatrices[16];
};