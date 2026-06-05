#pragma once
#include "core/GlobalUniforms.h"
#include "core/UniformBuffer.h"
#include "core/rendering/Mesh.h"
#include "helpers/Shader.h"
#include <glm/glm.hpp>
#include <vector>

class Renderer;

struct PointLight
{
    bool enabled = true;
    glm::vec3 position;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
    float constant = 1.0f;
    float linear = 0.09f;
    float quadratic = 0.032f;
    float radius = 1.0f;
    unsigned int depthMap = 0;
};

struct SpotLight
{
    bool enabled = true;
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

class LightManager
{
  public:
    LightManager();
    void SetDirectional(const glm::vec3 &dir, const glm::vec3 &ambient, const glm::vec3 &diffuse,
                        const glm::vec3 &specular);

    void AddPointLight(const PointLight &pl);
    void AddSpotLight(const SpotLight &sl);
    float CalculatePointLightRadius(const PointLight &light);
    void RecalculatePointLightRadiuses();
    void ClearPointLights();
    void ClearSpotLights();
    glm::vec3 GetDirectionalLightDir()
    {
        return dir;
    }

    // Getters for Editor
    glm::vec3 &GetDirLightDirection()
    {
        return dir;
    }
    glm::vec3 &GetDirLightAmbient()
    {
        return dirAmbient;
    }
    glm::vec3 &GetDirLightDiffuse()
    {
        return dirDiffuse;
    }
    glm::vec3 &GetDirLightSpecular()
    {
        return dirSpec;
    }

    // Apply stored lights to the currently used shader
    void ApplyToShader(Shader &shader, Renderer &renderer, const glm::mat4 &view, const glm::mat4 &proj);

    void InitUBO();
    void UploadLightsToGPU();

    void ShowDebugLights(bool show)
    {
        showDebugSpheres = show;
    }
    std::vector<PointLight> points;
    std::vector<SpotLight> spots;
    void RenderDebugLights(const glm::mat4 &view, const glm::mat4 &proj);

  private:
    std::unique_ptr<UniformBuffer> lightUBO;
    glm::vec3 dir = glm::vec3(0.0f);
    glm::vec3 dirAmbient = glm::vec3(0.0f);
    glm::vec3 dirDiffuse = glm::vec3(0.0f);
    glm::vec3 dirSpec = glm::vec3(0.0f);
    // show debug spheres for lights
    bool showDebugSpheres = true;
    std::shared_ptr<Mesh> debugSphere;
    std::shared_ptr<Shader> debugShader;
};