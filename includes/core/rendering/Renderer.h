#pragma once
#include <glm/glm.hpp>
#include <memory>
#include "helpers/Shader.h"
#include "core/LightManager.h"
#include "core/rendering/Mesh.h"
#include "core/rendering/GlobalUBO.h"

class Model;
class Camera;
class Framebuffer;
class PostProcessingPipeline;
struct Entity;
struct RenderSettings
{
    bool showNormals = false;
    bool outlineEnabled = false;
    glm::vec3 outlineColor = glm::vec3(1.0f, 1.0f, 1.0f);
};


class Renderer 
{
public:
    void BeginScene(const Camera &camera, 
                    GlobalUBO &ubo, 
                    LightManager &lightManager, 
                    float aspectRatio);

    void RenderScene(std::vector<std::shared_ptr<Entity>> entities, 
                    Camera &camera, 
                    LightManager &lightManager,
                    Framebuffer &sceneFBO, 
                    PostProcessingPipeline &postProcessor,
                    bool wireFrame = false);
        
   // SubmitMesh Overloads
    void SubmitMesh(const glm::mat4& model, 
                    const Mesh& mesh, 
                    const std::shared_ptr<Shader>& shader, 
                    const std::shared_ptr<Material>& mat);

    void SubmitMesh(const glm::mat4& model, 
                    const Mesh& mesh, 
                    const std::shared_ptr<Shader>& shader, 
                    const std::shared_ptr<Material>& mat,
                    const RenderSettings& overrides);

    // SubmitModel Overloads
    void SubmitModel(const glm::mat4& model, 
                     Model& modelObj, 
                     const std::shared_ptr<Shader>& shader);

    void SubmitModel(const glm::mat4& model, 
                     Model& modelObj, 
                     const std::shared_ptr<Shader>& shader,
                     const RenderSettings& settings);

    // Draw a Model N times
    void SubmitInstancedModel(Model& modelObj, 
                              const std::shared_ptr<Shader>& shader, 
                              int instanceCount);

    void SubmitSkybox(const Mesh& mesh, 
                      const std::shared_ptr<Shader>& shader, 
                      const std::shared_ptr<Material>& mat);
    void EndScene();

    std::shared_ptr<Shader> outlineShader;
    std::shared_ptr<Shader> normalShader;

private:
    glm::mat4 viewMatrix;
    glm::mat4 projMatrix;
    glm::vec3 viewPosition;
};
