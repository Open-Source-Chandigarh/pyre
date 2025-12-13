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
#include "core/rendering/Material.h"

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

void Renderer::RenderScene(std::vector<std::shared_ptr<Entity>> entities, Camera &camera, std::shared_ptr<LightManager> lightManager,
        std::shared_ptr<Framebuffer> sceneFBO, std::shared_ptr<PostProcessingPipeline> postProcessor,
        bool wireFrame)
{
    std::vector<std::shared_ptr<Entity>> transparentEntities;
    std::vector<std::shared_ptr<Entity>> opaqueEntities;
    std::shared_ptr<Entity> skyboxEntity = nullptr;

    // split lists and detect skybox (only use first skybox entity)
    for (auto& e : entities)
    {
        if (!e) continue;
        if (e->skyboxComp) {
            // prefer the first found skybox and skip adding it to the lists
            if (!skyboxEntity) skyboxEntity = e;
            continue; // don't treat skybox as normal geometry
        }

        // guard: some entities (e.g. models) might not have a material; just treat them opaque
       if (e->meshComp) 
       {
            if (e->meshComp->material && e->meshComp->material->isTransparent)
                transparentEntities.push_back(e);
            else
                opaqueEntities.push_back(e);
        }
        else if (e->modelComp) 
        {
            opaqueEntities.push_back(e);
        }
    }

    // Helper lambda to draw a single entity
    auto DrawEntity = [&](Entity* e) 
    {
        glm::mat4 modelMatrix = e->transform.GetModelMatrix();

        if (e->meshComp) 
        {
            SubmitMesh(modelMatrix, 
                       *e->meshComp->mesh, 
                       e->meshComp->shader, 
                       e->meshComp->material);
        }
        else if (e->modelComp) 
        {
            if (e->modelComp->instanceCount > 1) 
            {
                SubmitInstancedModel(*e->modelComp->model, 
                                     e->modelComp->shader, 
                                     e->modelComp->instanceCount);
            } else 
            {
                SubmitModel(modelMatrix, 
                            *e->modelComp->model, 
                            e->modelComp->shader, e->modelComp->renderSettings); 
            }
        }
    };

    // sort transparent back-to-front
    glm::vec3 camPosition = camera.Position;
    std::sort(transparentEntities.begin(), transparentEntities.end(),
    [&camPosition](const std::shared_ptr<Entity> a, const std::shared_ptr<Entity> b) 
    {
        float da = glm::dot(camPosition - a->transform.position, camPosition - a->transform.position);
        float db = glm::dot(camPosition - b->transform.position, camPosition - b->transform.position);
        return da > db; // far first
    });

    // If we have scene FBO and post processing (and not wireframe), render to FBO then postprocess
    if (sceneFBO && postProcessor && !wireFrame)
    {
        sceneFBO->Bind();
        glEnable(GL_DEPTH_TEST);
        glClearColor(0.05f, 0.05f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        for (auto& e : opaqueEntities)
        {
            DrawEntity(e.get());
        }
       
        // Render skybox before doing post processing
        if (skyboxEntity) 
        {
            SubmitSkybox(*skyboxEntity->skyboxComp->mesh, 
                         skyboxEntity->skyboxComp->shader,
                         skyboxEntity->skyboxComp->material);
        }

        if (lightManager) 
        {
            lightManager->RenderDebugLights(viewMatrix, projMatrix);
        }

        for (auto& e : transparentEntities)
        {
            DrawEntity(e.get());
        }

        sceneFBO -> ResolveToScreen();

        // Run post-processing pipeline on scene texture
        GLuint processed = postProcessor->Apply(sceneFBO->GetIntermediateTexture());

        // Blit final result to screen
        postProcessor->DrawToScreen(processed);

        // Restore default GL state expected by main loop if needed
        glEnable(GL_DEPTH_TEST);
    }
    else
    {
        for (auto& e : opaqueEntities)
        {
           DrawEntity(e.get());
        }
        if (skyboxEntity) 
        {
            SubmitSkybox(*skyboxEntity->skyboxComp->mesh, 
                         skyboxEntity->skyboxComp->shader,
                         skyboxEntity->skyboxComp->material);
        }
        if (lightManager) 
        {
            lightManager->RenderDebugLights(viewMatrix, projMatrix);
        }
        for (auto& e : transparentEntities)
        {
            DrawEntity(e.get());
        }
    } 
}

void Renderer::SubmitInstancedModel(Model& modelObj, 
                                    const std::shared_ptr<Shader>& shader, 
                                    int instanceCount)
{
    if (!shader) return;
    shader->use();
    
    // Set Globals (We don't set 'model' matrix because VBO handles it)
    if (shader->hasUniform("view"))       shader->setMat4("view", viewMatrix);
    if (shader->hasUniform("projection")) shader->setMat4("projection", projMatrix);
    if (shader->hasUniform("viewPos"))    shader->setVec3("viewPos", viewPosition);

    // Iterate meshes in the model
    for (const auto& entry : modelObj.GetMeshes())
    {
        entry.mesh->DrawInstanced(*shader, *entry.material, instanceCount);
    }
}

// Handles Main Pass + Debug Passes (Normals, Outlines)
void Renderer::SubmitMesh(const glm::mat4& model, 
                          const Mesh& mesh, 
                          const std::shared_ptr<Shader>& shader, 
                          const std::shared_ptr<Material>& mat,
                          const RenderSettings& overrides)
{
    if (!shader || !mat) return;

    // Combine Material flags with Override flags 
    bool doShowNormals  = mat->showNormals    || overrides.showNormals;
    bool doOutline      = mat->outlineEnabled || overrides.outlineEnabled;
    glm::vec3 outlineCol = (overrides.outlineEnabled) ? overrides.outlineColor : mat->outlineColor;

    // If outlining, write to stencil buffer If not, don't touch stencil
    if (doOutline) {
        glStencilFunc(GL_ALWAYS, 1, 0xFF);
        glStencilMask(0xFF);
    } else {
        glStencilMask(0x00); 
    }

    shader->use();
    
    // Set Standard Uniforms
    if (shader->hasUniform("model"))      shader->setMat4("model", model);
    if (shader->hasUniform("view"))       shader->setMat4("view", viewMatrix);
    if (shader->hasUniform("projection")) shader->setMat4("projection", projMatrix);
    if (shader->hasUniform("viewPos"))    shader->setVec3("viewPos", viewPosition);

    mesh.Draw(*shader, *mat);

    // Outline (Optional)
    if (doOutline && outlineShader)
    {
        glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
        glStencilMask(0x00);

        outlineShader->use();
        
        // Scale up slightly
        glm::mat4 outlineModel = glm::scale(model, glm::vec3(1.03f)); 

        outlineShader->setMat4("model", outlineModel);
        outlineShader->setMat4("view", viewMatrix);
        outlineShader->setMat4("projection", projMatrix);
        outlineShader->setVec3("color", outlineCol);

        mesh.DrawSimple();

        // Restore State
        glStencilMask(0xFF);
        glStencilFunc(GL_ALWAYS, 0, 0xFF);
    }

    // Normals (Optional)
    if (doShowNormals && normalShader)
    {
        normalShader->use();
        
        // Normals need the model matrix (to stick to surface)
        normalShader->setMat4("model", model);
        normalShader->setMat4("view", viewMatrix);
        normalShader->setMat4("projection", projMatrix);
        
        // Yellow color
        if(normalShader->hasUniform("color")) 
            normalShader->setVec3("color", glm::vec3(1.0f, 1.0f, 0.0f));

        mesh.DrawSimple();
    }

    // Reset active texture to be safe for next draw call
    glActiveTexture(GL_TEXTURE0);
}

// Just delegates to SubmitMesh for every piece of the model
void Renderer::SubmitModel(const glm::mat4& modelMatrix, 
                           Model& modelObj, 
                           const std::shared_ptr<Shader>& shader,
                           const RenderSettings& settings)
{
    if (!shader) return;

    // We don't need to bind the shader here because SubmitMesh handles it.
    // However, for optimization, we *could* bind global uniforms once here.
    // For simplicity/correctness with overrides, let SubmitMesh handle state changes.

    const auto& meshes = modelObj.GetMeshes();
    for (const auto& entry : meshes)
    {
        SubmitMesh(modelMatrix, 
                   *entry.mesh, 
                   shader, 
                   entry.material, 
                   settings); // Pass the model-wide settings down!
    }
}

void Renderer::SubmitMesh(const glm::mat4 &model, const Mesh &mesh, 
    const std::shared_ptr<Shader> &shader, const std::shared_ptr<Material>& mat)
{
    SubmitMesh(model, mesh, shader, mat, RenderSettings()); // Call main logic with defaults
}

void Renderer::SubmitModel(const glm::mat4 &model, Model &modelObj, 
    const std::shared_ptr<Shader> &shader)
{
    SubmitModel(model, modelObj, shader, RenderSettings()); // Call main logic with defaults
}

void Renderer::SubmitSkybox(const Mesh &mesh, 
                            const std::shared_ptr<Shader> &shader, 
                            const std::shared_ptr<Material> &mat)
{
    if (!shader || !mat) return;

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

    shader->use();

    // Set uniforms only if present (shader may read from UBO instead)
    if (shader->hasUniform("view")) shader->setMat4("view", viewNoTrans);
    if (shader->hasUniform("projection")) shader->setMat4("projection", proj);
    if (shader->hasUniform("skybox")) shader->setInt("skybox", 0);

    // bind cubemap to unit 0 (match the sampler set above if any)
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapID);

    // Draw the sky mesh (DrawSimple should not depend on material)
    mesh.DrawSimple();

    // restore state
    glDepthFunc(GL_LESS);
    glActiveTexture(GL_TEXTURE0);
}

void Renderer::EndScene()
{
    // Future batching or post effects
}