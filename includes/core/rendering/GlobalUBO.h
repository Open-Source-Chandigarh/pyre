#pragma once
#include <glm/glm.hpp>

struct LightUBO
{
    glm::mat4 view;
    glm::mat4 proj;
    glm::vec4 viewPos; // w as padding

    int numPointLights;
    int numSpotLights;
    int pad0;
    int pad1;

    glm::vec4 dir_direction;
    glm::vec4 dir_ambient;
    glm::vec4 dir_diffuse;
    glm::vec4 dir_specular;

    glm::vec4 point_position[8];
    glm::vec4 point_ambient[8];
    glm::vec4 point_diffuse[8];
    glm::vec4 point_specular[8];
    glm::vec4 point_params[8];

    glm::vec4 spot_position[4];
    glm::vec4 spot_direction[4];
    glm::vec4 spot_cutoffs[4];
    glm::vec4 spot_ambient[4];
    glm::vec4 spot_diffuse[4];
    glm::vec4 spot_specular[4];
    glm::vec4 spot_params[4];
};

void CreateGlobalUBO();
void UpdateGlobalUBO(const LightUBO& data);
void DestroyGlobalUBO();