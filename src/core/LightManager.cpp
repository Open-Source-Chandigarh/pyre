#include "core/LightManager.h"
#include "core/Entity.h"
#include "core/ResourceManager.h"
#include "core/rendering/geometry/GeometryFactory.h"
#include <sstream>
#include <iostream>

LightManager::LightManager()
{
    debugSphere = GeometryFactory::CreateSphere(0.2f, 4, 2);
    debugShader = ResourceManager::LoadShader("unlit",
        "shaders/singleColor.vs", "shaders/singleColor.fs");
}

void LightManager::SetDirectional(const glm::vec3& d,
    const glm::vec3& ambient,
    const glm::vec3& diffuse,
    const glm::vec3& specular)
{
    dir = d; dirAmbient = ambient; dirDiffuse = diffuse; dirSpec = specular;
}

void LightManager::AddPointLight(const PointLight& pl) {
    points.push_back(pl);
}

void LightManager::ClearPointLights() {
    points.clear();
}

void LightManager::AddSpotLight(const SpotLight& sl)
{
    spots.push_back(sl);
}

void LightManager::ClearSpotLights()
{
    spots.clear();
}

static constexpr int GLSL_MAX_POINT_LIGHTS = 8; // must match shader (#define MAX_POINT_LIGHTS 8)
static constexpr int GLSL_MAX_SPOT_LIGHTS = 4; // must match shader (#define MAX_SPOT_LIGHTS 4)

void LightManager::ApplyToShader(Shader& shader, Renderer& renderer,
    const glm::mat4& view, const glm::mat4& proj) 
{
    shader.use();

    shader.setVec3("dirLight.direction", dir);
    shader.setVec3("dirLight.ambient", dirAmbient);
    shader.setVec3("dirLight.diffuse", dirDiffuse);
    shader.setVec3("dirLight.specular", dirSpec);

    // clamp sizes to shader capacity
    int pointCount = static_cast<int>(std::min(points.size(), (size_t)GLSL_MAX_POINT_LIGHTS));
    int spotCount = static_cast<int>(std::min(spots.size(), (size_t)GLSL_MAX_SPOT_LIGHTS));

    shader.setInt("numPointLights", pointCount);
    for (int i = 0; i < pointCount; ++i) 
    {
        const auto& p = points[i];
        std::string base = "pointLights[" + std::to_string(i) + "].";
        shader.setVec3((base + "position").c_str(), p.position);
        shader.setVec3((base + "ambient").c_str(), p.ambient);
        shader.setVec3((base + "diffuse").c_str(), p.diffuse);
        shader.setVec3((base + "specular").c_str(), p.specular);
        shader.setFloat((base + "constant").c_str(), p.constant);
        shader.setFloat((base + "linear").c_str(), p.linear);
        shader.setFloat((base + "quadratic").c_str(), p.quadratic);


    }

    shader.setInt("numSpotLights", spotCount);
    for (int i = 0; i < spotCount; ++i) {
        const auto& s = spots[i];
        std::string base = "spotLights[" + std::to_string(i) + "]."; // IMPORTANT: "spotLights" not "pointLights"
        shader.setVec3((base + "position").c_str(), s.position);
        shader.setVec3((base + "direction").c_str(), s.direction);
        shader.setFloat((base + "innerCutOff").c_str(), s.innerCutOff);
        shader.setFloat((base + "outerCutOff").c_str(), s.outerCutOff);
        shader.setVec3((base + "ambient").c_str(), s.ambient);
        shader.setVec3((base + "diffuse").c_str(), s.diffuse);
        shader.setVec3((base + "specular").c_str(), s.specular);
        shader.setFloat((base + "constant").c_str(), s.constant);
        shader.setFloat((base + "linear").c_str(), s.linear);
        shader.setFloat((base + "quadratic").c_str(), s.quadratic);
    }

    RenderDebugLights(renderer, view, proj);
}

void LightManager::RenderDebugLights(Renderer& renderer,
    const glm::mat4& view, const glm::mat4& proj)
{
    if (!showDebugSpheres) return;
    if (!debugShader) return;

    debugShader->use();
    debugShader->setMat4("view", view);
    debugShader->setMat4("projection", proj);

    for (const auto& p : points)
    {
        glm::mat4 model = glm::translate(glm::mat4(1.0f), p.position);
        debugShader->setMat4("model", model);
        debugShader->setVec3("color", p.diffuse);
        debugSphere.DrawSimple();
    }

}