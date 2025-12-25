#pragma once
#include <glm/glm.hpp>

// binding point 0
// updates every frame
struct CameraData
{
    glm::mat4 view;       // 64 bytes
    glm::mat4 proj;       // 64 bytes
    glm::vec3 viewPos;    // 12 bytes
    float padding;        // 4 bytes (align to 16 bytes)
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

// binding point 2
// updates every frame
struct ShadowData
{
    glm::mat4 lightSpaceMatrices[16];
};