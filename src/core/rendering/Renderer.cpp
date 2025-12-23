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

// prepares the renderer for a new frame.
// calculates view/proj matrices and uploads light data to the gpu.
void Renderer::BeginScene(const Camera &camera, GlobalUBO &ubo, 
        LightManager &lightManager, float aspectRatio)
{
    // camera math
    // calculate standard view/projection matrices based on camera state.
    viewMatrix = camera.GetViewMatrix();
    projMatrix = glm::perspective(glm::radians(camera.Zoom), 
                                  aspectRatio, camera.Near, camera.Far);
    viewPosition = camera.Position;

    // reset gl state
    // enable depth/stencil testing so geometry overlaps correctly.
    glEnable(GL_STENCIL_TEST);
    glEnable(GL_DEPTH_TEST);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
    glStencilMask(0xFF); // enable writing to stencil buffer

    // clear the canvas (color, depth, and stencil bits)
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    // load resources
    if(!outlineShader) 
        outlineShader = ResourceManager::LoadShader("outline", 
        "shaders/common/singleColor.vs", 
        "shaders/common/singleColor.fs");
    
    if (!depthShader)
        depthShader = ResourceManager::LoadShader("depth", 
            "shaders/shadows/depth.vs", 
            "shaders/shadows/depth.fs");
    
    if(!normalShader)
        normalShader = ResourceManager::LoadShader("normal", 
        "shaders/common/normal.vs", 
        "shaders/common/normal.fs", 
        "shaders/common/normal.gs");
    
    if (!shadowFBO) 
        shadowFBO = std::make_unique<Framebuffer>(2048, 2048, true, false, false);

    // lighting
    // upload all point/spot/dir lights to the uniform buffer object (ubo).
    lightManager.UploadToUBO(ubo, viewMatrix, projMatrix, viewPosition);
}

// generic helper to draw a list of entities.
// reused for both main pass and shadow pass.
void Renderer::RenderPass(const std::vector<std::shared_ptr<Entity>> &entities, 
                    const glm::mat4 &view, 
                    const glm::mat4 &proj,
                    std::shared_ptr<Shader> shaderOverride)
{
    glm::mat4 oldView = viewMatrix;
    glm::mat4 oldProj = projMatrix;
    viewMatrix = view;
    projMatrix = proj;
    auto DrawEntity = [&](Entity* e) 
    {
        glm::mat4 modelMatrix = e->transform.GetModelMatrix();
        std::shared_ptr<Shader> currentShader;

        // if we are overriding the shader (Shadow Pass), we force outlines off.
        RenderSettings currentSettings; 
        
        // copy existing settings if they exist (for Models)
        if (e->modelComp) currentSettings = e->modelComp->renderSettings;
        
        if (shaderOverride) 
        {
            // force disable extra features that ruin shadow maps
            currentSettings.outlineEnabled = false;
            currentSettings.showNormals = false;
        }

        if (e->meshComp)
        {
            currentShader = (shaderOverride) ? shaderOverride : e->meshComp->shader;

            if (currentShader)
            {
                // pass currentSettings to enforce the override (disable outlines)
                SubmitMesh(modelMatrix, 
                           *e->meshComp->mesh, 
                           currentShader, 
                           e->meshComp->material,
                           currentSettings); 
            }
        }
        else if (e->modelComp) 
        {
            currentShader = (shaderOverride) ? shaderOverride : e->modelComp->shader;
            
            if (currentShader)
            {
                if (e->modelComp->instanceCount > 1) 
                {
                    SubmitInstancedModel(*e->modelComp->model, 
                                         currentShader, 
                                         e->modelComp->instanceCount);
                } 
                else 
                {
                    // pass currentSettings to enforce the override
                    SubmitModel(modelMatrix, 
                                *e->modelComp->model, 
                                currentShader, 
                                currentSettings); 
                }
            }
        }
    };

    for (auto& e : entities)
    {
        // skip transparent objects for shadow pass
        if (shaderOverride && e->meshComp && e->meshComp->material && e->meshComp->material->isTransparent) 
            continue;

        DrawEntity(e.get());
    }

    viewMatrix = oldView;
    projMatrix = oldProj;
}

// the main orchestration function.
// handles sorting, fbos, multiple passes, and post-processing.
void Renderer::RenderScene(std::vector<std::shared_ptr<Entity>> entities, 
                           Camera &camera, 
                           LightManager &lightManager,
                           Framebuffer &sceneFBO, 
                           PostProcessingPipeline &postProcessor,
                           bool wireFrame,
                           glm::vec3 clearColor)
{
    // lists to separate renderable types
    std::vector<std::shared_ptr<Entity>> transparentEntities;
    std::vector<std::shared_ptr<Entity>> opaqueEntities;
    std::shared_ptr<Entity> skyboxEntity = nullptr;

    // 1. filtering
    // separate skybox, transparent, and opaque objects.
    for (auto& e : entities)
    {
        if (!e) continue;
        
        // grab the first skybox we find and skip adding it to normal lists
        if (e->skyboxComp) {
            if (!skyboxEntity) skyboxEntity = e;
            continue;
        }

        bool isTransparent = false;
        if (e->meshComp && e->meshComp->material && e->meshComp->material->isTransparent) 
            isTransparent = true;
        
        if (isTransparent) transparentEntities.push_back(e);
        else opaqueEntities.push_back(e);
    }

    // 2. sorting
    // sort transparent objects back-to-front. 
    // standard painter's algorithm to fix blending artifacts.
    glm::vec3 camPosition = camera.Position;
    std::sort(transparentEntities.begin(), transparentEntities.end(),
    [&camPosition](const std::shared_ptr<Entity> a, const std::shared_ptr<Entity> b) 
    {
        float da = glm::dot(camPosition - a->transform.position, camPosition - a->transform.position);
        float db = glm::dot(camPosition - b->transform.position, camPosition - b->transform.position);
        return da > db; // farther objects first
    });

    // render pass 1: shadow mapping
    
    // we calculate where the position of the area which is 10 units in front of the player/camera
    glm::vec3 flatFront = glm::normalize(glm::vec3(camera.Front.x, 0.0f, camera.Front.z));
    glm::vec3 targetPos = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 lightDir = glm::normalize(lightManager.GetDirectionalLightDir());

    // backup 80 units from the player along the direction of the light to 
    // get the position of the directional light or where should the virtual camera sit
    glm::vec3 lightPos = targetPos - (lightDir * 80.0f);

    // construct matrices from lights pov
    glm::mat4 lightView = glm::lookAt(lightPos, targetPos, glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 lightProj = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, 1.0f, 100.0f);
    // convert our scene space matrices to light space
    lightSpaceMatrix = lightProj * lightView;

    // prep shadow rendering
    depthShader->use();
    depthShader->setMat4("lightSpaceMatrix", lightSpaceMatrix);

    shadowFBO->Bind();
    glViewport(0, 0, shadowFBO->Width(), shadowFBO->Height());
    glClear(GL_DEPTH_BUFFER_BIT);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);

    // execute the draw call for opaque objects only
    RenderPass(opaqueEntities, lightView, lightProj, depthShader);

    Framebuffer::Unbind();
    // reset viewport to screen size for next pass
    glViewport(0, 0, sceneFBO.Width(), sceneFBO.Height());
    glCullFace(GL_BACK);

    // render pass 2: main lighting pass

    if (!wireFrame)
    {
        // bind the scene framebuffer to capture colors for post-processing
        sceneFBO.Bind();
        glEnable(GL_DEPTH_TEST);
        glClearColor(clearColor.r, clearColor.g, clearColor.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        // bind depth texture for shadow mapping
        glActiveTexture(GL_TEXTURE10);
        glBindTexture(GL_TEXTURE_2D, shadowFBO->GetDepthTexture());

        // a. draw opaque geometry
        // this writes to the depth buffer so subsequent draws clip correctly.
        RenderPass(opaqueEntities, viewMatrix, projMatrix, nullptr);
        
        // b. draw skybox
        // drawn after opaque so we don't process pixels covered by geometry.
        if (skyboxEntity) 
        {
            SubmitSkybox(*skyboxEntity->skyboxComp->mesh, 
                         skyboxEntity->skyboxComp->shader,
                         skyboxEntity->skyboxComp->material);
        }

        // c. debug lights
        // draw little lightbulbs/spheres where the point lights are.
        lightManager.RenderDebugLights(viewMatrix, projMatrix);

        // d. draw transparent geometry
        // drawn last so they blend over opaque objects and skybox.
        RenderPass(transparentEntities, viewMatrix, projMatrix, nullptr);

        // resolve msaa (if enabled)
        sceneFBO.ResolveToScreen();

        // e. post-processing
        // take the texture from the FBO and run it through the effects pipeline.
        GLuint processed = postProcessor.Apply(sceneFBO.GetIntermediateTexture());
        postProcessor.DrawToScreen(processed);

        glEnable(GL_DEPTH_TEST); // restore state just in case
    }
    else
    {
        // wireframe mode logic (skip fbo/post-process for clarity)
        RenderPass(opaqueEntities, viewMatrix, projMatrix, nullptr);
        
        if (skyboxEntity) 
            SubmitSkybox(*skyboxEntity->skyboxComp->mesh, 
                         skyboxEntity->skyboxComp->shader, 
                         skyboxEntity->skyboxComp->material);
                         
        lightManager.RenderDebugLights(viewMatrix, projMatrix);
        RenderPass(transparentEntities, viewMatrix, projMatrix, nullptr);
    }
}

// handles hardware instancing.
// draws the model 'instanceCount' times in a single draw call.
void Renderer::SubmitInstancedModel(Model& modelObj, 
                                    const std::shared_ptr<Shader>& shader, 
                                    int instanceCount)
{
    if (!shader) return;
    shader->use();
    
    // set camera globals.
    // note: we do NOT set the 'model' matrix here because that data comes
    // from the instance VBO (vertex buffer) setup in the Mesh class.
    if (shader->hasUniform("view"))       shader->setMat4("view", viewMatrix);
    if (shader->hasUniform("projection")) shader->setMat4("projection", projMatrix);
    if (shader->hasUniform("viewPos"))    shader->setVec3("viewPos", viewPosition);
    if (shader->hasUniform("shadowMap")) 
    {
        shader->setInt("shadowMap", 10);
        glActiveTexture(GL_TEXTURE10);
        glBindTexture(GL_TEXTURE_2D, shadowFBO->GetDepthTexture());
    }
    if (shader->hasUniform("lightSpaceMatrix")) 
    {
        shader->setMat4("lightSpaceMatrix", lightSpaceMatrix);
    }

    // iterate meshes in the model
    for (const auto& entry : modelObj.GetMeshes())
    {
        entry.mesh->DrawInstanced(*shader, *entry.material, instanceCount);
    }
}

// the core draw function.
// handles material binding, stencil outlines, and normal debug visualization.
void Renderer::SubmitMesh(const glm::mat4& model, 
                          const Mesh& mesh, 
                          const std::shared_ptr<Shader>& shader, 
                          const std::shared_ptr<Material>& mat,
                          const RenderSettings& overrides)
{
    if (!shader || !mat) return;

    // merge material flags with any overrides provided by the model component
    bool doShowNormals  = mat->showNormals    || overrides.showNormals;
    bool doOutline      = mat->outlineEnabled || overrides.outlineEnabled;
    glm::vec3 outlineCol = (overrides.outlineEnabled) ? overrides.outlineColor : mat->outlineColor;

    // stencil logic
    // always enable writing to stencil buffer (mask 0xff).
    // if outline enabled write 1.
    // if outline disabled write 0. 
    // this ensures non-outlined objects clean the stencil buffer for their pixels.
    glStencilMask(0xFF); 
    if (doOutline)
        glStencilFunc(GL_ALWAYS, 1, 0xFF);
    else 
        glStencilFunc(GL_ALWAYS, 0, 0xFF);

    shader->use();
    if(shader->hasUniform("shadowMap")) 
    {
        shader->setInt("shadowMap", 10); // Tell shader to look at unit 10
        glActiveTexture(GL_TEXTURE10);
        glBindTexture(GL_TEXTURE_2D, shadowFBO->GetDepthTexture()); 
    }
    // upload standard transformation matrices
    if (shader->hasUniform("model"))           shader->setMat4("model", model);
    if (shader->hasUniform("view"))            shader->setMat4("view", viewMatrix);
    if (shader->hasUniform("projection"))      shader->setMat4("projection", projMatrix);
    if (shader->hasUniform("viewPos"))         shader->setVec3("viewPos", viewPosition);
    if (shader->hasUniform("lightSpaceMatrix")) shader->setMat4("lightSpaceMatrix", lightSpaceMatrix);

    // standard culling (don't draw back faces)
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    // main draw call
    mesh.Draw(*shader, *mat);

    // outline pass (optional)
    if (doOutline && outlineShader)
    {
        // logic: only draw where stencil value is not 1.
        // since we just drew the object at 1, this prevents drawing over the object itself.
        glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
        glStencilMask(0x00); // don't write to stencil

        outlineShader->use();
        
        // scale the object up slightly (e.g. 1.03x) to create the border
        glm::mat4 outlineModel = glm::scale(model, glm::vec3(1.03f)); 

        outlineShader->setMat4("model", outlineModel);
        outlineShader->setMat4("view", viewMatrix);
        outlineShader->setMat4("projection", projMatrix);
        outlineShader->setVec3("color", outlineCol);

        mesh.DrawSimple(); // draw without materials/textures

        // cleanup stencil state
        glStencilMask(0xFF);
        glStencilFunc(GL_ALWAYS, 0, 0xFF);
    }

    // normals debug pass (optional)
    if (doShowNormals && normalShader)
    {
        normalShader->use();
        
        // geometry shader needs model matrix to place normals correctly
        normalShader->setMat4("model", model);
        normalShader->setMat4("view", viewMatrix);
        normalShader->setMat4("projection", projMatrix);
        
        if(normalShader->hasUniform("color")) 
            normalShader->setVec3("color", glm::vec3(1.0f, 1.0f, 0.0f)); // yellow

        mesh.DrawSimple();
    }

    // cleanup
    glActiveTexture(GL_TEXTURE0);
}

// wrapper to draw a full model (which may contain multiple meshes).
void Renderer::SubmitModel(const glm::mat4& modelMatrix, 
                           Model& modelObj, 
                           const std::shared_ptr<Shader>& shader,
                           const RenderSettings& settings)
{
    if (!shader) return;

    // iterate through all sub-meshes (e.g. wheels, body, windows)
    const auto& meshes = modelObj.GetMeshes();
    for (const auto& entry : meshes)
    {
        SubmitMesh(modelMatrix, 
                   *entry.mesh, 
                   shader, 
                   entry.material, 
                   settings);
    }
}

// helper overload with default settings
void Renderer::SubmitMesh(const glm::mat4 &model, const Mesh &mesh, 
    const std::shared_ptr<Shader> &shader, const std::shared_ptr<Material>& mat)
{
    SubmitMesh(model, mesh, shader, mat, RenderSettings());
}

// helper overload with default settings
void Renderer::SubmitModel(const glm::mat4 &model, Model &modelObj, 
    const std::shared_ptr<Shader> &shader)
{
    SubmitModel(model, modelObj, shader, RenderSettings()); 
}

// specialized skybox rendering.
// requires depth buffer tricks to render "behind" everything else.
void Renderer::SubmitSkybox(const Mesh &mesh, 
                            const std::shared_ptr<Shader> &shader, 
                            const std::shared_ptr<Material> &mat)
{
    if (!shader || !mat) return;

    // locate the cubemap texture in the material
    std::shared_ptr<Texture> cubeTex = nullptr;
    auto it = mat->textures.find("skybox");
    if (it != mat->textures.end()) cubeTex = it->second;
    else {
        // fallback search for any cubemap
        for (const auto& kv : mat->textures) {
            if (kv.second && kv.second->type == TextureType::TEX_CUBEMAP) {
                cubeTex = kv.second;
                break;
            }
        }
    }
    if (!cubeTex) return;

    // magic trick: set depth function to LEQUAL.
    // the skybox shader sets Z to 1.0 (max depth).
    // LEQUAL ensures it passes only if nothing else is in front of it.
    glDepthFunc(GL_LEQUAL);
    glDisable(GL_CULL_FACE); // render inside of cube

    // strip translation from view matrix so skybox doesn't move when we walk
    glm::mat4 viewNoTrans = glm::mat4(glm::mat3(viewMatrix));
    
    shader->use();
    if (shader->hasUniform("view")) shader->setMat4("view", viewNoTrans);
    if (shader->hasUniform("projection")) shader->setMat4("projection", projMatrix);
    if (shader->hasUniform("skybox")) shader->setInt("skybox", 0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubeTex->ID);

    mesh.DrawSimple();

    // restore standard state
    glEnable(GL_CULL_FACE); 
    glCullFace(GL_BACK);
    glDepthFunc(GL_LESS);
    glActiveTexture(GL_TEXTURE0);
}

void Renderer::EndScene()
{
    // currently empty. 
    // will be used for batch flushing or ui rendering later.
}