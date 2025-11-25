#pragma once
#include <glm/glm.hpp>
#include <memory>
#include "helpers/Shader.h"
#include "core/rendering/Mesh.h"

class Model;
class Camera;
class Framebuffer;
class PostProcessingPipeline;
struct Entity;

class Renderer 
{
public:
    void BeginScene(const glm::mat4& view, const glm::mat4& projection, 
        const glm::vec3& viewPos);
    void RenderScene(std::vector<Entity*> entities, Camera& camera,
        Framebuffer* sceneFBO = nullptr, PostProcessingPipeline* postProcessor = nullptr,
        bool wireFrame = false);
    void SubmitMesh(const glm::mat4& model,
        const Mesh& mesh,
        const std::shared_ptr<Shader>& shader, const std::shared_ptr<Material>& mat);
    void SubmitModel(const glm::mat4& model, Model& modelObj, 
        const std::shared_ptr<Shader>& shader);
    void SubmitSkybox(Entity* skyEntity);
    void EndScene();

private:
    glm::mat4 viewMatrix;
    glm::mat4 projMatrix;
    glm::vec3 viewPosition;
};
