#include <glad/glad.h>
#include <algorithm>
#include <glm/glm.hpp>
#include "core/rendering/Renderer.h"
#include "core/rendering/Model.h"
#include "core/ResourceManager.h"
#include "core/Entity.h"
#include "helpers/camera.h"

void Renderer::BeginScene(const glm::mat4& view, const glm::mat4& projection,
    const glm::vec3& viewPos)
{
    glEnable(GL_STENCIL_TEST);
    glEnable(GL_DEPTH_TEST);
    // Default stencil op: replace stencil on depth pass (we'll set func per pass below)
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

    // Clear color, depth and stencil at frame start to avoid stale values.
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    viewMatrix = view;
    projMatrix = projection;
    viewPosition = viewPos;
}

void Renderer::RenderScene(std::vector<Entity*> entities, Camera &camera)
{
    std::vector<Entity*> transparentEntities;

    for (auto& e : entities)
    {
        if (!e -> meshRenderer.material->isTransparent)
            e -> Render(*this);
        else
            transparentEntities.push_back(e);
    }

    glm::vec3 camPosition = camera.Position;
    std::sort(transparentEntities.begin(), transparentEntities.end(),
        [&camPosition](const Entity* a, const Entity* b) {
            // squared distances (cheaper than sqrt)
            float da = glm::dot(camPosition - a->transform.position, camPosition - a->transform.position);
            float db = glm::dot(camPosition - b->transform.position, camPosition - b->transform.position);
            return da > db; // far first (strict)
        });

    for (auto& e : transparentEntities)
    {
        e->Render(*this);
    }
}
// --------------------------------------------
// SubmitMesh – Draws a single mesh with material
// --------------------------------------------
void Renderer::SubmitMesh(const glm::mat4& model,
    const Mesh& mesh,
    const std::shared_ptr<Shader>& shader, const std::shared_ptr<Material>& mat)
{
    if (!shader) return;

    switch (mat->cullMode)
    {
    case CullMode::Back:
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        break;
    case CullMode::Front:
        glEnable(GL_CULL_FACE);
        glCullFace(GL_FRONT);
        break;
    case CullMode::None:
        glDisable(GL_CULL_FACE);
        break;
    }

    // --- NON-OUTLINE: simple draw (ensure stencil not written) ---
    if (!mat->outlineEnabled)
    {
        // Prevent writing to stencil for regular objects
        glStencilMask(0x00);        // disable writing to stencil
      
        shader->use();
        shader->setMat4("model", model);
        shader->setMat4("view", viewMatrix);
        shader->setMat4("projection", projMatrix);
        shader->setVec3("viewPos", viewPosition);

        mesh.Draw(*shader, *mat);
        return;
    }

    // --- OUTLINE: two-pass technique ---

    // 1) Render object and write stencil = 1 where fragments pass depth.
    glStencilFunc(GL_ALWAYS, 1, 0xFF); // stencil value becomes 1 where object draws
    glStencilMask(0xFF);               // allow writing to stencil buffer
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

    // Ensure normal rendering state
    glEnable(GL_DEPTH_TEST);
  
    shader->use();
    shader->setMat4("model", model);
    shader->setMat4("view", viewMatrix);
    shader->setMat4("projection", projMatrix);
    shader->setVec3("viewPos", viewPosition);

    // Draw the actual object (this writes stencil=1 where fragments drew)
    mesh.Draw(*shader, *mat);

    // 2) Outline pass: draw where stencil != 1.

    // Now configure stencil test for outline; don't write to stencil
    glStencilFunc(GL_NOTEQUAL, 1, 0xFF); // draw only where stencil != 1
    glStencilMask(0x00);                 // disable stencil writes for outline
  
    // Slightly scale the model for rim size (smaller factor avoids self-intersection)
    const float outlineScale = 1.04f; // tweak between 1.01 - 1.1 depending on mesh
    glm::mat4 outlineModel = glm::scale(model, glm::vec3(outlineScale));

    std::shared_ptr<Shader> outlineShader = ResourceManager::LoadShader("simple",
        "shaders/singleColor.vs", "shaders/singleColor.fs");
    if (outlineShader)
    {
        outlineShader->use();
        outlineShader->setMat4("model", outlineModel);
        outlineShader->setMat4("view", viewMatrix);
        outlineShader->setMat4("projection", projMatrix);
        outlineShader->setVec3("color", mat->outlineColor);

        // Draw raw geometry for the rim (no textures/material)
        mesh.DrawSimple();
    }

    // Restore stencil defaults for subsequent draws
    glStencilMask(0xFF);
    glStencilFunc(GL_ALWAYS, 0, 0xFF);
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