#include <glad/glad.h>
#include "core/rendering/Renderer.h"
#include "core/rendering/Model.h"

void Renderer::BeginScene(const glm::mat4& view, const glm::mat4& projection,
    const glm::vec3& viewPos)
{
    viewMatrix = view;
    projMatrix = projection;
    viewPosition = viewPos;
}

// --------------------------------------------
// SubmitMesh – Draws a single mesh with material
// --------------------------------------------
void Renderer::SubmitMesh(const glm::mat4& model,
    const Mesh& mesh,
    const std::shared_ptr<Shader>& shader, const std::shared_ptr<Material>& mat)
{
    if (!shader) return;

    shader->use();
    shader->setMat4("model", model);
    shader->setMat4("view", viewMatrix);
    shader->setMat4("projection", projMatrix);
    shader->setVec3("viewPos", viewPosition);

    mesh.Draw(*shader, *mat);
}

// --------------------------------------------
// SubmitModel – Draws an entire model (with per-mesh materials)
// --------------------------------------------
void Renderer::SubmitModel(const glm::mat4& model,
    Model& modelObj,
    const std::shared_ptr<Shader>& shader)
{
    if (!shader) return;

    shader->use();
    shader->setMat4("model", model);
    shader->setMat4("view", viewMatrix);
    shader->setMat4("projection", projMatrix);
    shader->setVec3("viewPos", viewPosition);
    
    modelObj.Draw(*shader);
}

void Renderer::EndScene()
{
    // Future batching or post effects
}