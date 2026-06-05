#pragma once
#include <glm/glm.hpp>

// binding point 0
// updates every frame
struct CameraData
{
    glm::mat4 view;    // 64 bytes
    glm::mat4 proj;    // 64 bytes
    glm::vec3 viewPos; // 12 bytes
    float padding;     // 4 bytes (align to 16 bytes)
};

// binding point 1
// updates when lights change
struct LightsData
{
    // directional light
    glm::vec4 dir_direction;
    glm::vec4 dir_ambient;
    glm::vec4 dir_diffuse;
    glm::vec4 dir_specular;

    // counts
    int numPointLights;
    int numSpotLights;
    int _pad0;
    int _pad1;

    // point light arrays (max 8)
    glm::vec4 point_position[8];
    glm::vec4 point_ambient[8];
    glm::vec4 point_diffuse[8];
    glm::vec4 point_specular[8];
    glm::vec4 point_params[8]; // x=const, y=lin, z=quad

    // spot light arrays (max 4)
    glm::vec4 spot_position[4];
    glm::vec4 spot_direction[4];
    glm::vec4 spot_cutoffs[4];
    glm::vec4 spot_ambient[4];
    glm::vec4 spot_diffuse[4];
    glm::vec4 spot_specular[4];
    glm::vec4 spot_params[4];
};

// binding point 2: CSM
// updates every frame
struct ShadowData
{
    glm::mat4 lightSpaceMatrices[16];
    glm::vec4 cascadePlaneDistances[4]; // Packed: .x=0, .y=1, .z=2, .w=3...
    int cascadeCount = 0;
    float shadowFarPlane = 0.0f;
    float _padCSM1; // 4 bytes
    float _padCSM2; // 4 bytes (Align to 16)
};

// binding point 3: point shadows
// updates every frame once per light
struct PointShadowData
{
    glm::mat4 shadowMatrices[6]; // 384 bytes
    glm::vec4 lightPos;          // 16 bytes
    float farPlane;              // 4 bytes
    float _padPoint1;
    float _padPoint2;
    float _padPoint3; // 12 bytes padding total
};