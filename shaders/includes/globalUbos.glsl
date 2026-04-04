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

// binding 3: Point Shadow Data
layout (std140, binding = 3) uniform PointShadowData
{
    mat4 shadowMatrices[6]; // 384 bytes
    vec4 lightPos;          // 16 bytes
    float farPlane;         // 4 bytes
    float _padPoint1;
    float _padPoint2;
    float _padPoint3;       // 12 bytes padding total
};