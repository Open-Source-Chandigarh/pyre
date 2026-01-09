#pragma once
#include <glm/glm.hpp>
#include <vector>
#include "helpers/Shader.h"
#include "core/rendering/Mesh.h"
#include "core/GlobalUniforms.h"
#include "core/UniformBuffer.h"

class Renderer;

struct PointLight 
{
    glm::vec3 position;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
    float constant = 1.0f;
    float linear = 0.09f;
    float quadratic = 0.032f;
    unsigned int depthMap = 0;
};

struct SpotLight 
{
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

    LightManager();
    void SetDirectional(const glm::vec3& dir,
        const glm::vec3& ambient,
        const glm::vec3& diffuse,
        const glm::vec3& specular);

    void AddPointLight(const PointLight& pl);
    void AddSpotLight(const SpotLight& sl);
    void ClearPointLights();
    void ClearSpotLights();
    glm::vec3 GetDirectionalLightDir() { return dir; }

    // Apply stored lights to the currently used shader
    void ApplyToShader(Shader& shader, Renderer& renderer,
        const glm::mat4& view, const glm::mat4& proj);

    void InitUBO();
    void UploadLightsToGPU();

    void ShowDebugLights(bool show) { showDebugSpheres = show; }
    std::vector<PointLight> points;
    std::vector<SpotLight> spots;
    void RenderDebugLights(const glm::mat4& view, const glm::mat4& proj);

private:
    std::unique_ptr<UniformBuffer> lightUBO;
    glm::vec3 dir = glm::vec3(0.0f);
    glm::vec3 dirAmbient = glm::vec3(0.0f);
    glm::vec3 dirDiffuse = glm::vec3(0.0f);
    glm::vec3 dirSpec = glm::vec3(0.0f);
    // show debug spheres for lights
    bool showDebugSpheres = true;
    Mesh debugSphere;
    std::shared_ptr<Shader> debugShader;
};