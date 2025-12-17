#pragma once
#include <glm/glm.hpp>
#include <vector>
#include "helpers/Shader.h"
#include "core/rendering/Mesh.h"

class Renderer;
class GlobalUBO;

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

    LightManager();
    void SetDirectional(const glm::vec3& dir,
        const glm::vec3& ambient,
        const glm::vec3& diffuse,
        const glm::vec3& specular);

    void AddPointLight(const PointLight& pl);
    void AddSpotLight(const SpotLight& sl);
    void ClearPointLights();
    void ClearSpotLights();

    // Apply stored lights to the currently used shader
    void ApplyToShader(Shader& shader, Renderer& renderer,
        const glm::mat4& view, const glm::mat4& proj);

    void UploadToUBO(GlobalUBO &ubo,
                    const glm::mat4& view, 
                    const glm::mat4& proj, 
                    const glm::vec3& cameraPos);

    void ShowDebugLights(bool show) { showDebugSpheres = show; }
    std::vector<PointLight> points;
    std::vector<SpotLight> spots;
    void RenderDebugLights(const glm::mat4& view, const glm::mat4& proj);

private:
    glm::vec3 dir = glm::vec3(0.0f);
    glm::vec3 dirAmbient = glm::vec3(0.0f);
    glm::vec3 dirDiffuse = glm::vec3(0.0f);
    glm::vec3 dirSpec = glm::vec3(0.0f);
    bool showDebugSpheres = true;
    Mesh debugSphere;
    std::shared_ptr<Shader> debugShader;
    // Show debug spheres for lights
};