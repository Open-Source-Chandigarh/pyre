#pragma once
#include <glm/glm.hpp>
#include <vector>
#include "helpers/shaderClass.h"

struct PointLight {
    glm::vec3 position;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
    float constant = 1.0f;
    float linear = 0.09f;
    float quadratic = 0.032f;
};

struct SpotLight {
    glm::vec3 position;
    glm::vec3 direction;

    float innerCutOff;
    float outerCutOff;

    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;

    float constant;
    float linear;
    float quadratic;
};

class LightManager {
public:
    void SetDirectional(const glm::vec3& dir,
        const glm::vec3& ambient,
        const glm::vec3& diffuse,
        const glm::vec3& specular);

    void AddPointLight(const PointLight& pl);
    void AddSpotLight(const SpotLight& sl);
    void ClearPointLights();
    void ClearSpotLights();

    // Apply stored lights to the currently used shader
    void ApplyToShader(Shader& shader);

    std::vector<PointLight> points;
    std::vector<SpotLight> spots;

private:
    glm::vec3 dir = glm::vec3(0.0f);
    glm::vec3 dirAmbient = glm::vec3(0.0f);
    glm::vec3 dirDiffuse = glm::vec3(0.0f);
    glm::vec3 dirSpec = glm::vec3(0.0f);
};