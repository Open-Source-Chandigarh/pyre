#include <glad/glad.h>
#include <algorithm>
#include <glm/glm.hpp>
#include "core/rendering/Renderer.h"
#include "core/rendering/Model.h"
#include "core/ResourceManager.h"
#include "core/Entity.h"
#include "helpers/camera.h"
#include "core/postprocessing/PostProcessingPipeline.h"
#include "core/rendering/Framebuffer.h"

void Renderer::BeginScene(const glm::mat4& view, const glm::mat4& projection,
    const glm::vec3& viewPos)
{
    glEnable(GL_STENCIL_TEST);
    glEnable(GL_DEPTH_TEST);
    // Default stencil op: replace stencil on depth pass (we'll set func per pass below)
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

    // Clear color, depth and stencil at frame start to avoid stale values.
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    outlineShader = ResourceManager::LoadShader("outline",
        "shaders/common/singleColor.vs", "shaders/common/singleColor.fs");

    normalShader = ResourceManager::LoadShader("normal_debug", 
        "shaders/common/normal.vs", "shaders/common/normal.fs", "shaders/common/normal.gs");

    viewMatrix = view;
    projMatrix = projection;
    viewPosition = viewPos;
}

void Renderer::RenderScene(std::vector<Entity*> entities, Camera& camera,
    Framebuffer* sceneFBO, PostProcessingPipeline* postProcessor,
    bool wireFrame)
{
    std::vector<Entity*> transparentEntities;
    std::vector<Entity*> opaqueEntities;
    Entity* skyboxEntity = nullptr;

    // split lists and detect skybox (only use first skybox entity)
    for (auto& e : entities)
    {
        if (!e) continue;
        if (e->type == Entity::Type::SkyBox) {
            // prefer the first found skybox and skip adding it to the lists
            if (!skyboxEntity) skyboxEntity = e;
            continue; // don't treat skybox as normal geometry
        }

        // guard: some entities (e.g. models) might not have a material; just treat them opaque
        if (e->meshRenderer.material && e->meshRenderer.material->isTransparent)
            transparentEntities.push_back(e);
        else
            opaqueEntities.push_back(e);
    }

    // sort transparent back-to-front
    glm::vec3 camPosition = camera.Position;
    std::sort(transparentEntities.begin(), transparentEntities.end(),
        [&camPosition](const Entity* a, const Entity* b) {
            float da = glm::dot(camPosition - a->transform.position, camPosition - a->transform.position);
            float db = glm::dot(camPosition - b->transform.position, camPosition - b->transform.position);
            return da > db; // far first
        });


    // Draw skybox first (if exists)

    // If we have scene FBO and post processing (and not wireframe), render to FBO then postprocess
    if (sceneFBO && postProcessor && !wireFrame)
    {
        sceneFBO->Bind();
        glEnable(GL_DEPTH_TEST);
        glClearColor(0.05f, 0.05f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        for (auto& e : opaqueEntities)
        {
            e->Render(*this);
        }
        for (auto& e : transparentEntities)
        {
            e->Render(*this);
        }
        // Render skybox before doing post processing
        if (skyboxEntity) SubmitSkybox(skyboxEntity);

        // 2) Run post-processing pipeline on scene texture
        GLuint processed = postProcessor->Apply(sceneFBO->GetColorTexture());

        // 3) Blit final result to screen
        postProcessor->DrawToScreen(processed);

        // Restore default GL state expected by main loop if needed
        glEnable(GL_DEPTH_TEST);
    }
    else
    {
        for (auto& e : opaqueEntities)
        {
            e->Render(*this);
        }
        if (skyboxEntity) {
            SubmitSkybox(skyboxEntity);
        }
        for (auto& e : transparentEntities)
        {
            e->Render(*this);
        }
    } 
}

// --------------------------------------------
// SubmitMesh Draws a single mesh with material
// --------------------------------------------
void Renderer::SubmitMesh(const glm::mat4& model,
    const Mesh& mesh,
    const std::shared_ptr<Shader>& shader, const std::shared_ptr<Material>& mat)
{
    if (!shader) return;

    // --- NON-OUTLINE: simple draw (ensure stencil not written) ---
    if (!mat->outlineEnabled)
    {
        glStencilMask(0x00); // disable stencil writes

        shader->use();

        // set per-object 'model' if shader expects it
        if (shader->hasUniform("model")) shader->setMat4("model", model);

        // For view/proj/viewPos: set only if shader has legacy uniforms.
        if (shader->hasUniform("view"))       shader->setMat4("view", viewMatrix);
        if (shader->hasUniform("projection")) shader->setMat4("projection", projMatrix);
        if (shader->hasUniform("viewPos"))    shader->setVec3("viewPos", viewPosition);

        mesh.Draw(*shader, *mat);
        return;
    }

    // --- OUTLINE: two-pass technique ---

    // 1) Render object and write stencil = 1 where fragments pass depth.
    glStencilFunc(GL_ALWAYS, 1, 0xFF);
    glStencilMask(0xFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

    glEnable(GL_DEPTH_TEST);

    shader->use();
    if (shader->hasUniform("model")) shader->setMat4("model", model);
    if (shader->hasUniform("view"))       shader->setMat4("view", viewMatrix);
    if (shader->hasUniform("projection")) shader->setMat4("projection", projMatrix);
    if (shader->hasUniform("viewPos"))    shader->setVec3("viewPos", viewPosition);

    mesh.Draw(*shader, *mat);

    // 2) Outline pass: draw where stencil != 1.
    glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
    glStencilMask(0x00);

    const float outlineScale = 1.04f;
    glm::mat4 outlineModel = glm::scale(model, glm::vec3(outlineScale));

    if (outlineShader)
    {
        outlineShader->use();
        if (outlineShader->hasUniform("model")) outlineShader->setMat4("model", outlineModel);
        if (outlineShader->hasUniform("view")) outlineShader->setMat4("view", viewMatrix);
        if (outlineShader->hasUniform("projection")) outlineShader->setMat4("projection", projMatrix);
        if (outlineShader->hasUniform("color")) outlineShader->setVec3("color", mat->outlineColor);

        // Draw raw geometry for the rim (no material applied)
        mesh.DrawSimple();
    }

    if(mat->showNormals && normalShader)
    {
        normalShader->use();
        if (normalShader->hasUniform("model")) normalShader->setMat4("model", model);
        if (normalShader->hasUniform("view")) normalShader->setMat4("view", viewMatrix);
        if (normalShader->hasUniform("projection")) normalShader->setMat4("projection", projMatrix);

        mesh.DrawSimple();
    }

    // Restore stencil defaults
    glStencilMask(0xFF);
    glStencilFunc(GL_ALWAYS, 0, 0xFF);

    // reset active texture unit to 0 to be safe (Material::ApplyToShader already does this)
    glActiveTexture(GL_TEXTURE0);
}

  
// --------------------------------------------
// SubmitModel Draws an entire model (with per-mesh materials)
// --------------------------------------------
void Renderer::SubmitModel(const glm::mat4& model,
    Model& modelObj,
    const std::shared_ptr<Shader>& shader)
{
    if (!shader) return;

    shader->use();

    if (shader->hasUniform("model")) shader->setMat4("model", model);
    if (shader->hasUniform("view")) shader->setMat4("view", viewMatrix);
    if (shader->hasUniform("projection")) shader->setMat4("projection", projMatrix);
    if (shader->hasUniform("viewPos")) shader->setVec3("viewPos", viewPosition);

    modelObj.Draw(*shader);
}

void Renderer::SubmitSkybox(Entity* skyEntity)
{
    if (!skyEntity) return;
    if (!skyEntity->meshRenderer.mesh || !skyEntity->meshRenderer.shader) return;
    auto mat = skyEntity->meshRenderer.material;
    if (!mat) return;

    // Find cubemap texture in material by convention:
    // prefer "skybox" key, else first Texture with type TEX_CUBEMAP
    std::shared_ptr<Texture> cubeTex = nullptr;
    auto it = mat->textures.find("skybox");
    if (it != mat->textures.end()) cubeTex = it->second;
    else {
        for (const auto& kv : mat->textures) {
            if (kv.second && kv.second->type == TextureType::TEX_CUBEMAP) {
                cubeTex = kv.second;
                break;
            }
        }
    }
    if (!cubeTex) return;

    GLuint cubemapID = cubeTex->ID;

    // Depth func so skybox passes at far plane
    glDepthFunc(GL_LEQUAL);

    // Use view without translation so skybox is camera-centered
    glm::mat4 viewNoTrans = glm::mat4(glm::mat3(viewMatrix));
    glm::mat4 proj = projMatrix;

    auto skyShader = skyEntity->meshRenderer.shader;
    skyShader->use();

    // Set uniforms only if present (shader may read from UBO instead)
    if (skyShader->hasUniform("view")) skyShader->setMat4("view", viewNoTrans);
    if (skyShader->hasUniform("projection")) skyShader->setMat4("projection", proj);
    if (skyShader->hasUniform("skybox")) skyShader->setInt("skybox", 0);

    // bind cubemap to unit 0 (match the sampler set above if any)
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapID);

    // Draw the sky mesh (DrawSimple should not depend on material)
    skyEntity->meshRenderer.mesh->DrawSimple();

    // restore state
    glDepthFunc(GL_LESS);
    glActiveTexture(GL_TEXTURE0);
}

void Renderer::EndScene()
{
    // Future batching or post effects
}