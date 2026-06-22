#include "core/rendering/Renderer.h"
#include "core/Constants.h"
#include "core/Entity.h"
#include "core/ResourceManager.h"
#include "core/postprocessing/PostProcessingPipeline.h"
#include "core/rendering/Framebuffer.h"
#include "core/rendering/Material.h"
#include "core/rendering/Model.h"
#include "core/rendering/geometry/GeometryFactory.h"
#include "helpers/camera.h"
#include <algorithm>
#include <glad/glad.h>
#include <glm/glm.hpp>

#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include "imgui.h"

Renderer::Renderer()
{
    defaultMat = std::make_shared<Material>();
    defaultMat->floats["material_shininess"] = 32.0f;
    defaultMat->floats["material_reflectivity"] = 0.0f;
    defaultMat->vec3s["material_diffuseColor"] = glm::vec3(1.0f);
    ResourceManager::GenerateBRDFLUT();
}

Renderer::~Renderer()
{
    // clean up raw vertex array/buffer objects
    if (quadVAO != 0)
        glDeleteVertexArrays(1, &quadVAO);
    if (quadVBO != 0)
        glDeleteBuffers(1, &quadVBO);
    // clean up raw procedural textures
    if (noiseTexture != 0)
        glDeleteTextures(1, &noiseTexture);
}

void Renderer::SetupCameraGlobals(const Camera &camera, float aspectRatio)
{
    // we calculate the standard view and projection matrices based on the camera's current state
    // these will be used for almost every render pass except the shadow map generation
    // also we store aspect ratio for csm calculations later
    currentAspectRatio = aspectRatio;
    cameraFarPlane = camera.Far;
    cameraNearPlane = camera.Near;
    viewMatrix = camera.GetViewMatrix();
    projMatrix = glm::perspective(glm::radians(camera.Zoom), aspectRatio, camera.Near, camera.Far);
    viewPosition = camera.Position;

    if (!cameraUBO)
    {
        cameraUBO = std::make_unique<UniformBuffer>(sizeof(CameraData), Bindings::UBO_CAMERA);
    }

    CameraData camData{};
    camData.view = viewMatrix;
    camData.proj = projMatrix;
    camData.viewPos = viewPosition;
    cameraUBO->UploadData(&camData, sizeof(CameraData));
}

void Renderer::ResetGlState()
{
    // we need to enable depth testing so objects obscure each other correctly based on distance
    // and stencil testing is required for our outline selection effects later on
    glEnable(GL_STENCIL_TEST);
    glEnable(GL_DEPTH_TEST);

    // configuring the stencil op to replace values when the depth test passes allows us to create masks
    // 0xFF enables writing to the stencil buffer
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
    glStencilMask(0xFF);

    // finally we clear the color, depth, and stencil bits to prepare the canvas for a fresh frame
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

void Renderer::LoadRequiredShaders()
{
    // we perform a lazy load here to ensure these essential internal shaders are available
    // this prevents crashes if the user forgot to manually load 'outline', 'depth', or 'normal' shaders
    if (!outlineShader)
    {
        outlineShader =
            ResourceManager::LoadShader("outline", "shaders/common/singleColor.vs", "shaders/common/singleColor.fs");
    }

    if (!depthShader)
    {
        depthShader = ResourceManager::LoadShader("depth", "shaders/shadows/depth.vs", "shaders/shadows/depth.fs",
                                                  "shaders/shadows/depth.gs");
    }

    if (!pointLightSphere)
    {
        pointLightSphere = GeometryFactory::CreateSphere(1.0f, 32, 16);
    }

    if (!pointLightVolumeShader)
    {
        pointLightVolumeShader = ResourceManager::LoadShader("pointLightVolume", "shaders/deferred/pointLightVolume.vs",
                                                             "shaders/deferred/pointLightVolume.fs");
    }

    if (!normalShader)
    {
        normalShader = ResourceManager::LoadShader("normal", "shaders/common/normal.vs", "shaders/common/normal.fs",
                                                   "shaders/common/normal.gs");
    }
}

void Renderer::EnsureGBuffer(unsigned int width, unsigned int height)
{
    if (!gBufferFBO || gBufferFBO->Width() != width || gBufferFBO->Height() != height)
    {
        // 3 color attachments, 1 layer, not multisampled, not a cubemap
        gBufferFBO = std::make_unique<Framebuffer>(width, height, true, false, true, 1, false, 4);
    }

    if (!gBufferShader)
    {
        gBufferShader =
            ResourceManager::LoadShader("gbuffer", "shaders/deferred/gbuffer.vs", "shaders/deferred/gbuffer.fs");
    }
}

void Renderer::InitSSAO()
{
    if (!ssaoKernel.empty())
        return;

    std::uniform_real_distribution<float> randomFloats(-1.0, 1.0);
    std::default_random_engine generator;

    ssaoKernel.clear();

    for (unsigned int i = 0; i < 64; i++)
    {
        glm::vec3 sample(randomFloats(generator), randomFloats(generator),
                         randomFloats(generator) * 0.5 +
                             0.5); // convert the z to 0.0 to 1.0 to make a hemisphere and not a sphere

        sample = glm::normalize(sample); // normalize to convert the points from a sqaure to a hemisphere
        sample *= randomFloats(generator) * 0.5 +
                  0.5; // multiple by a random float to move points from the boundaries to the inside of our hemisphere

        // accerlerating interpolation function to make sure we have more points closer to the surface origin
        float scale = float(i) / 64.0; // this gives us an exponential curve
        scale = glm::mix(
            0.1, 1.0,
            scale * scale); // lerp gives us smaller numbers to multiple with when scale is smaller (i is smaller)
        sample *= scale;
        ssaoKernel.push_back(sample);
    }

    std::vector<glm::vec3> randomNoise;
    for (unsigned int i = 0; i < 16; i++)
    {
        glm::vec3 noise(randomFloats(generator), // x between -1.0 and 1.0
                        randomFloats(generator), // y between -1.0 and 1.0
                        0.0f // z locked to 0.0 because we don't wana rotate sideways and tilt the hemisphere inside the
                             // surface point
        );
        randomNoise.push_back(noise);
    }

    glGenTextures(1, &noiseTexture);
    glBindTexture(GL_TEXTURE_2D, noiseTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, 4, 4, 0, GL_RGB, GL_FLOAT, &randomNoise[0]);
    // MAG and MIN filters MUST be GL_NEAREST
    // with linear, the GPU will
    // blur the random numbers together, destroying the mathematical vectors
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    // WRAP_S and WRAP_T MUST be GL_REPEAT to tile the 4x4 grid across the screen
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
}

void Renderer::EnsureShadowBuffer()
{
    // the shadow map framebuffer must exist before we try to render into it
    // we initialize it with a high resolution to minimize pixelation artifacts
    if (!shadowFBO)
    {
        // 3 splits = 4 layers (0-10, 10-200, 200-500, 500-infinity)
        unsigned int layers = shadowCascadeLevels.size() + 1;
        shadowFBO = std::make_unique<Framebuffer>(2048, 2048, true, false, false, layers);
    }

    CreateShadowUBO();
}

void Renderer::EnsureSSAOBuffer(unsigned int width, unsigned int height)
{
    if (!ssaoFBO || ssaoFBO->Width() != width || ssaoFBO->Height() != height)
    {
        ssaoFBO = std::make_unique<Framebuffer>(width, height, false, false, true, 1, false, 1);
        ssaoBlurFBO = std::make_unique<Framebuffer>(width, height, false, false, true, 1, false, 1);
    }

    if (!ssaoShader)
    {
        ssaoShader =
            ResourceManager::LoadShader("ssao", "shaders/common/simpleTexture.vs", "shaders/postprocessing/ssao.fs");
    }

    if (!ssaoBlurShader)
    {
        ssaoBlurShader = ResourceManager::LoadShader("ssaoBlur", "shaders/common/simpleTexture.vs",
                                                     "shaders/postprocessing/ssaoBlur.fs");
    }
}

void Renderer::EnsurePointShadowBuffer()
{
    if (!pointShadowFBO)
    {
        pointShadowFBO = std::make_unique<Framebuffer>(512, 512, true, false, false, 8, true);

        pointShadowShader =
            ResourceManager::LoadShader("pointShadow", "shaders/shadows/pointDepth.vs", "shaders/shadows/pointDepth.fs",
                                        "shaders/shadows/pointDepth.gs");
    }

    CreatePointShadowUBO();
}

void Renderer::CreateShadowUBO()
{
    if (shadowUBO)
        return;
    // binding point 2, size of ShadowData struct
    shadowUBO = std::make_unique<UniformBuffer>(sizeof(ShadowData), Bindings::UBO_CSM_SHADOWS);
}

void Renderer::CreatePointShadowUBO()
{
    if (pointShadowUBO)
        return;
    // size of struct, binding point 3
    pointShadowUBO = std::make_unique<UniformBuffer>(sizeof(PointShadowData), Bindings::UBO_POINT_SHADOWS);
}

void Renderer::BeginScene(const Camera &camera, LightManager &lightManager, float aspectRatio)
{
    // this is the entry point for the frame where we prepare all global state
    SetupCameraGlobals(camera, aspectRatio);
    ResetGlState();
    LoadRequiredShaders();
    EnsureShadowBuffer();
    EnsurePointShadowBuffer();

    // uploading lighting data to the uniform buffer object ensures all shaders have access to light sources
    lightManager.UploadLightsToGPU();
}

void Renderer::EndScene()
{
    // currently this is a placeholder but it will eventually handle ui rendering or batch flushing
    // once we implement a 2d user interface system
}

void Renderer::InitQuad()
{
    if (quadVAO != 0)
        return;

    float quadVertices[] = {
        // Position (X, Y)    // UV (U, V)
        -1.0f, 1.0f,  0.0f, 1.0f, // Top-Left
        -1.0f, -1.0f, 0.0f, 0.0f, // Bottom-Left
        1.0f,  1.0f,  1.0f, 1.0f, // Top-Right
        1.0f,  -1.0f, 1.0f, 0.0f, // Bottom-Right
    };

    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *) 0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *) (2 * sizeof(float)));

    glBindVertexArray(0);
}

void Renderer::RenderQuad()
{
    if (quadVAO == 0)
        InitQuad();
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

void Renderer::DrawEntityInPass(Entity *e, std::shared_ptr<Shader> shaderOverride, bool isShadowPass)
{
    if (!e->renderComp)
        return;
    std::shared_ptr<Shader> currentShader = (shaderOverride) ? shaderOverride : e->renderComp->shader;
    if (!currentShader)
        return;
    std::vector<ModelNode> *nodesToDraw = &e->renderComp->nodes;

    // if this is a shadow pass and we have a proxy, we use the proxy
    if (isShadowPass && e->renderComp->shadowProxyModel)
        nodesToDraw = &e->renderComp->shadowProxyModel->nodes;

    for (const auto &node : *nodesToDraw)
    {
        if (!node.mesh)
            continue;
        // Get Base Material (from file)
        std::shared_ptr<Material> baseMat = node.baseMaterial;

        // Get Override Material (from Entity settings)
        std::shared_ptr<Material> overrideMat = e->renderComp->materialOverride;

        bool forceFwd = false;
        if (overrideMat && overrideMat->bools.count("forceForward"))
            forceFwd = overrideMat->GetBool("forceForward");
        else if (baseMat)
            forceFwd = baseMat->GetBool("forceForward");

        // Skip forward only objects in the GBuffer pass
        if (shaderOverride == gBufferShader && forceFwd)
            continue;

        // Transparency
        bool isTransparent = false;
        if (overrideMat && overrideMat->isTransparent)
            isTransparent = true;
        else if (baseMat && baseMat->isTransparent)
            isTransparent = true;
        if (shaderOverride && isTransparent)
            continue;

        // Shadows
        bool castShadows = true;
        if (overrideMat && overrideMat->bools.count("castShadows"))
            castShadows = overrideMat->GetBool("castShadows");
        else if (baseMat)
            castShadows = baseMat->GetBool("castShadows", true);
        if (isShadowPass && !castShadows)
            continue;

        // Calculate Transform
        glm::mat4 nodeMatrix = e->transform.GetModelMatrix() * node.localTransform;
        if (isShadowPass)
            SubmitMesh(nodeMatrix, *node.mesh, currentShader, nullptr, nullptr, e->renderComp->instanceCount, true);
        else
            SubmitMesh(nodeMatrix, *node.mesh, currentShader, baseMat, overrideMat, e->renderComp->instanceCount,
                       false);
    }
}

void Renderer::RenderPass(const std::vector<std::shared_ptr<Entity>> &entities, const glm::mat4 &view,
                          const glm::mat4 &proj, std::shared_ptr<Shader> shaderOverride, bool isShadowPass)
{
    // we back up the main camera matrices because this pass might use alternative matrices
    // for examples: we want to add a mirror that will draw the scene from mirror's perspective which will have
    // different view and proj matrices
    glm::mat4 oldView = viewMatrix;
    glm::mat4 oldProj = projMatrix;

    viewMatrix = view;
    projMatrix = proj;

    for (auto &e : entities)
    {
        if (!e->renderComp)
            continue;

        DrawEntityInPass(e.get(), shaderOverride, isShadowPass);
    }

    // restore the original camera state so subsequent passes don't render from the wrong perspective
    viewMatrix = oldView;
    projMatrix = oldProj;
}

void Renderer::RenderBatches(const std::shared_ptr<Shader> &overrideShader)
{
    bool isGBufferPass = (overrideShader == gBufferShader);
    bool isShadowPass = (overrideShader == depthShader || overrideShader == pointShadowShader);

    for (auto &shaderPair : opaqueBatch)
    {
        std::shared_ptr<Shader> activeShader = overrideShader ? overrideShader : shaderPair.first;
        if (!activeShader)
            continue;

        activeShader->use();

        if (activeShader->hasUniform("view"))
            activeShader->setMat4("view", viewMatrix);
        if (activeShader->hasUniform("projection"))
            activeShader->setMat4("projection", projMatrix);
        if (activeShader->hasUniform("viewPos"))
            activeShader->setVec3("viewPos", viewPosition);

        if (activeShader->hasUniform("shadowMap"))
        {
            activeShader->setInt("shadowMap", Bindings::TEX_SLOT_CSM_SHADOW);
            glActiveTexture(GL_TEXTURE0 + Bindings::TEX_SLOT_CSM_SHADOW);
            glBindTexture(GL_TEXTURE_2D, shadowFBO->GetDepthTexture());
        }
        if (activeShader->hasUniform("pointShadowMap"))
        {
            activeShader->setInt("pointShadowMap", Bindings::TEX_SLOT_POINT_SHADOW);
            glActiveTexture(GL_TEXTURE0 + Bindings::TEX_SLOT_POINT_SHADOW);
            glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, pointShadowFBO->GetDepthTexture());
        }
        activeShader->setFloat("iblStrength", iblStrength);

        if (activeShader->hasUniform("irradianceMap"))
        {
            activeShader->setInt("irradianceMap", 16);
            activeShader->setInt("prefilterMap", 17);
            activeShader->setInt("brdfLUT", 18);
            if (activeIrradianceMap != 0)
            {
                activeShader->setInt("hasIrradianceMap", 1);
                glActiveTexture(GL_TEXTURE0 + 16);
                glBindTexture(GL_TEXTURE_CUBE_MAP, activeIrradianceMap);
            }
            else
            {
                activeShader->setInt("hasIrradianceMap", 0);
            }

            if (activePrefilterMap != 0)
            {
                glActiveTexture(GL_TEXTURE0 + 17);
                glBindTexture(GL_TEXTURE_CUBE_MAP, activePrefilterMap);
            }

            auto brdfTex = ResourceManager::GetTexture("BRDF_LUT");
            if (brdfTex)
            {
                glActiveTexture(GL_TEXTURE0 + 18);
                glBindTexture(GL_TEXTURE_2D, brdfTex->ID);
            }
        }

        for (auto &baseMaterialPair : shaderPair.second)
        {
            std::shared_ptr<Material> baseMaterial = baseMaterialPair.first;
            if (!baseMaterial)
                baseMaterial = defaultMat;

            for (auto &overrideMaterialPair : baseMaterialPair.second)
            {
                std::shared_ptr<Material> overrideMaterial = overrideMaterialPair.first;
                bool forceFwd = false;
                if (overrideMaterial && overrideMaterial->bools.count("forceForward"))
                    forceFwd = overrideMaterial->GetBool("forceForward");
                else
                    forceFwd = baseMaterial->GetBool("forceForward");

                if (isGBufferPass && forceFwd)
                    continue;

                bool castShadows = true;
                if (overrideMaterial && overrideMaterial->bools.count("castShadows"))
                    castShadows = overrideMaterial->GetBool("castShadows");
                else
                    castShadows = baseMaterial->GetBool("castShadows", true);

                if (isShadowPass && !castShadows)
                    continue;

                bool wireframe = false;
                if (overrideMaterial && overrideMaterial->bools.count("wireframe"))
                    wireframe = overrideMaterial->GetBool("wireframe");
                else
                    wireframe = baseMaterial->GetBool("wireframe");

                CullMode currentCull = baseMaterial->cullMode;
                if (overrideMaterial && overrideMaterial->cullMode != CullMode::Back)
                    currentCull = overrideMaterial->cullMode;

                bool doOutline = forceOutlines && !isShadowPass;
                glm::vec3 outlineCol(1.0f);
                float bloomFactor = 0.0f;
                float outlineThickness = 0.05f;

                if (!isShadowPass)
                {
                    if (overrideMaterial && overrideMaterial->bools.count("outlineEnabled"))
                    {
                        doOutline = overrideMaterial->GetBool("outlineEnabled") || doOutline;
                        outlineCol = overrideMaterial->GetVec3("outlineColor");
                        bloomFactor = overrideMaterial->GetFloat("bloomFactor");
                        outlineThickness = overrideMaterial->GetFloat("outlineThickness", 0.05f);
                    }
                    else if (baseMaterial->GetBool("outlineEnabled"))
                    {
                        doOutline = true;
                        outlineCol = baseMaterial->GetVec3("outlineColor");
                        bloomFactor = baseMaterial->GetFloat("bloomFactor");
                        outlineThickness = baseMaterial->GetFloat("outlineThickness", 0.05f);
                    }
                }

                bool doNormals = false;
                if (overrideMaterial && overrideMaterial->bools.count("showNormals"))
                    doNormals = overrideMaterial->GetBool("showNormals");
                else
                    doNormals = baseMaterial->GetBool("showNormals");

                // bind textures if we are doing standard drawing (not depth/shadow passes)
                if (overrideShader == nullptr || overrideShader == gBufferShader)
                {
                    baseMaterial->ApplyToShader(*activeShader, false);
                    if (overrideMaterial)
                        overrideMaterial->ApplyToShader(*activeShader, true);
                }

                if (wireframe)
                    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

                if (currentCull == CullMode::None)
                    glDisable(GL_CULL_FACE);
                else
                {
                    glEnable(GL_CULL_FACE);
                    glCullFace((currentCull == CullMode::Front) ? GL_FRONT : GL_BACK);
                }

                ConfigureStencilForOutline(doOutline);

                for (auto &meshPair : overrideMaterialPair.second)
                {
                    Mesh *mesh = meshPair.first;
                    std::vector<glm::mat4> &matrices = meshPair.second;

                    if (!mesh || matrices.empty())
                        continue;

                    mesh->SetupInstancing(matrices);
                    mesh->DrawInstanced(*activeShader, matrices.size());

                    if (doOutline && activeShader != gBufferShader)
                    {
                        for (const auto &mat : matrices)
                        {
                            RenderMeshOutline(*mesh, mat, outlineCol, bloomFactor, outlineThickness);
                        }
                    }

                    if (doNormals && !isShadowPass)
                    {
                        for (const auto &mat : matrices)
                        {
                            RenderMeshNormals(*mesh, mat);
                        }
                    }
                }

                if (wireframe)
                    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

                if (currentCull != CullMode::Back)
                {
                    glEnable(GL_CULL_FACE);
                    glCullFace(GL_BACK);
                }

                ConfigureStencilForOutline(false);
            }
        }
    }

    glActiveTexture(GL_TEXTURE0);
}

void Renderer::RenderGeometryPass()
{
    gBufferFBO->Bind();
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glDisable(GL_BLEND);
    // render all opaque objects
    RenderBatches(gBufferShader);

    for (auto &e : opaqueEntities)
    {
        if (e->renderComp && e->renderComp->instanceCount > 1)
            DrawEntityInPass(e.get(), gBufferShader, false);
    }
    glEnable(GL_BLEND);
    Framebuffer::Unbind();
}

void Renderer::RenderSSAOPass(unsigned int width, unsigned int height)
{
    InitSSAO();
    EnsureSSAOBuffer(width, height);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    ssaoFBO->Bind();
    glViewport(0, 0, ssaoFBO->Width(), ssaoFBO->Height());
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f); // Default to white (no shadow)
    glClear(GL_COLOR_BUFFER_BIT);

    ssaoShader->use();
    // upload the 64 kernel points
    for (unsigned int i = 0; i < 64; ++i)
        ssaoShader->setVec3("samples[" + std::to_string(i) + "]", ssaoKernel[i]);

    ssaoShader->setFloat("radius", ssaoRadius);
    ssaoShader->setInt("gPosition", Bindings::TEX_SLOT_GBUFFER_POSITION);
    glActiveTexture(GL_TEXTURE0 + Bindings::TEX_SLOT_GBUFFER_POSITION);
    glBindTexture(GL_TEXTURE_2D, gBufferFBO->GetColorTexture(0));
    ssaoShader->setInt("gNormal", Bindings::TEX_SLOT_GBUFFER_NORMAL);
    glActiveTexture(GL_TEXTURE0 + Bindings::TEX_SLOT_GBUFFER_NORMAL);
    glBindTexture(GL_TEXTURE_2D, gBufferFBO->GetColorTexture(1));
    ssaoShader->setInt("texNoise", Bindings::TEX_SLOT_SSAO_NOISE);
    glActiveTexture(GL_TEXTURE0 + Bindings::TEX_SLOT_SSAO_NOISE);
    glBindTexture(GL_TEXTURE_2D, noiseTexture);
    RenderQuad();

    ssaoBlurFBO->Bind();
    glClear(GL_COLOR_BUFFER_BIT);
    ssaoBlurShader->use();
    ssaoBlurShader->setInt("ssaoInput", 14);
    glActiveTexture(GL_TEXTURE0 + 14);
    glBindTexture(GL_TEXTURE_2D, ssaoFBO->GetColorTexture(0));
    RenderQuad();

    glEnable(GL_DEPTH_TEST);
}

void Renderer::RenderDeferredLightingPass(LightManager &lightManager, Framebuffer &sceneFBO,
                                          std::shared_ptr<Entity> skybox)
{
    if (!deferredLightingShader)
    {
        deferredLightingShader = ResourceManager::LoadShader("deferredLighting", "shaders/deferred/lighting.vs",
                                                             "shaders/deferred/lighting.fs");
    }

    deferredLightingShader->use();

    // bind texture slots to the shader uniforms
    deferredLightingShader->setInt("gPosition", Bindings::TEX_SLOT_GBUFFER_POSITION);
    deferredLightingShader->setInt("gNormal", Bindings::TEX_SLOT_GBUFFER_NORMAL);
    deferredLightingShader->setInt("gAlbedoSpec", Bindings::TEX_SLOT_GBUFFER_ALBEDO);
    deferredLightingShader->setInt("shadowMap", Bindings::TEX_SLOT_CSM_SHADOW);
    deferredLightingShader->setInt("pointShadowMap", Bindings::TEX_SLOT_POINT_SHADOW);
    deferredLightingShader->setInt("ssaoMap", 12);

    deferredLightingShader->setFloat("iblStrength", iblStrength);

    deferredLightingShader->setInt("irradianceMap", 16);
    deferredLightingShader->setInt("prefilterMap", 17);
    deferredLightingShader->setInt("brdfLUT", 18);
    if (activeIrradianceMap != 0)
    {
        deferredLightingShader->setInt("hasIrradianceMap", 1);
        glActiveTexture(GL_TEXTURE0 + 16);
        glBindTexture(GL_TEXTURE_CUBE_MAP, activeIrradianceMap);
    }
    else
    {
        deferredLightingShader->setInt("hasIrradianceMap", 0);
    }

    if (activePrefilterMap != 0)
    {
        glActiveTexture(GL_TEXTURE0 + 17);
        glBindTexture(GL_TEXTURE_CUBE_MAP, activePrefilterMap);
    }

    auto brdfTex = ResourceManager::GetTexture("BRDF_LUT");
    if (brdfTex)
    {
        glActiveTexture(GL_TEXTURE0 + 18);
        glBindTexture(GL_TEXTURE_2D, brdfTex->ID);
    }

    deferredLightingShader->setInt("skyboxTexture", 15);
    if (skybox && skybox->skyboxComp && skybox->skyboxComp->material)
    {
        auto it = skybox->skyboxComp->material->textures.find("skybox");
        if (it != skybox->skyboxComp->material->textures.end())
        {
            glActiveTexture(GL_TEXTURE0 + 15);
            glBindTexture(GL_TEXTURE_CUBE_MAP, it->second->ID);
        }
    }

    // bind g-buffer textures
    glActiveTexture(GL_TEXTURE0 + Bindings::TEX_SLOT_GBUFFER_POSITION);
    glBindTexture(GL_TEXTURE_2D, gBufferFBO->GetColorTexture(0));

    glActiveTexture(GL_TEXTURE0 + Bindings::TEX_SLOT_GBUFFER_NORMAL);
    glBindTexture(GL_TEXTURE_2D, gBufferFBO->GetColorTexture(1));

    glActiveTexture(GL_TEXTURE0 + Bindings::TEX_SLOT_GBUFFER_ALBEDO);
    glBindTexture(GL_TEXTURE_2D, gBufferFBO->GetColorTexture(2));

    glActiveTexture(GL_TEXTURE0 + Bindings::TEX_SLOT_GBUFFER_EMISSIVE);
    glBindTexture(GL_TEXTURE_2D, gBufferFBO->GetColorTexture(3));

    // bind shadow maps
    glActiveTexture(GL_TEXTURE0 + Bindings::TEX_SLOT_CSM_SHADOW);
    glBindTexture(GL_TEXTURE_2D_ARRAY, shadowFBO->GetDepthTexture());

    // bind ssao map
    glActiveTexture(GL_TEXTURE0 + 12);
    glBindTexture(GL_TEXTURE_2D, ssaoBlurFBO->GetColorTexture(0));

    if (pointShadowFBO)
    {
        glActiveTexture(GL_TEXTURE0 + Bindings::TEX_SLOT_POINT_SHADOW);
        glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, pointShadowFBO->GetDepthTexture());
    }

    // disable depth testing (we are just drawing a flat 2D quad over the screen)
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    // draw full screen quad to resolve all lighting
    RenderQuad();

    glEnable(GL_DEPTH_TEST);
}

void Renderer::RenderLocalLightVolumes(LightManager &lightManager, Framebuffer &sceneFBO)
{
    sceneFBO.Bind();

    // we only want to read depth, never write to it here
    glDepthMask(GL_FALSE);

    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_ONE, GL_ONE);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT); // cull the front faces this makes it impossible for the camera near plane to clip the light
    glEnable(GL_DEPTH_CLAMP);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_GEQUAL); // only shade pixels that are equal or closer to camera than the back face of light distance
                            // from camera

    pointLightVolumeShader->use();

    glActiveTexture(GL_TEXTURE0 + Bindings::TEX_SLOT_GBUFFER_POSITION);
    glBindTexture(GL_TEXTURE_2D, gBufferFBO->GetColorTexture(0));

    glActiveTexture(GL_TEXTURE0 + Bindings::TEX_SLOT_GBUFFER_NORMAL);
    glBindTexture(GL_TEXTURE_2D, gBufferFBO->GetColorTexture(1));

    glActiveTexture(GL_TEXTURE0 + Bindings::TEX_SLOT_GBUFFER_ALBEDO);
    glBindTexture(GL_TEXTURE_2D, gBufferFBO->GetColorTexture(2));

    glActiveTexture(GL_TEXTURE0 + Bindings::TEX_SLOT_GBUFFER_EMISSIVE);
    glBindTexture(GL_TEXTURE_2D, gBufferFBO->GetColorTexture(3));

    if (pointShadowFBO)
    {
        glActiveTexture(GL_TEXTURE0 + Bindings::TEX_SLOT_POINT_SHADOW);
        glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, pointShadowFBO->GetDepthTexture());
        pointLightVolumeShader->setInt("pointShadowMap", Bindings::TEX_SLOT_POINT_SHADOW);
    }

    int shadowLayerIndex = 0;

    for (const auto &light : lightManager.points)
    {
        if (!light.enabled)
            continue;

        glm::mat4 model(1.0f);
        model = glm::translate(model, light.position);
        model = glm::scale(model, glm::vec3(light.radius));

        pointLightVolumeShader->setMat4("model", model);
        // pass the index so the shader knows which UBO array element and which shadow layer to use
        pointLightVolumeShader->setInt("lightIndex", shadowLayerIndex);
        pointLightSphere->DrawSimple();

        shadowLayerIndex++;
    }

    glDisable(GL_DEPTH_CLAMP);
    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);
    glCullFace(GL_BACK);
    glDepthFunc(GL_LESS);
}

void Renderer::CopyDepthBuffer(Framebuffer &source, Framebuffer &dest)
{
    // we are taking the depth data generated by the gBuffer pass and
    // copying it into the scene FBO so our forward rendered objects know what's in front of them
    glBindFramebuffer(GL_READ_FRAMEBUFFER, source.GetID());
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dest.GetID());

    glBlitFramebuffer(0, 0, source.Width(), source.Height(), 0, 0, dest.Width(), dest.Height(),
                      GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT, GL_NEAREST);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::CategorizeEntities(const std::vector<std::shared_ptr<Entity>> &source, std::shared_ptr<Entity> &skybox)
{
    opaqueBatch.clear();
    transparentEntities.clear();
    opaqueEntities.clear();

    // reset the active map at the start of every frame
    activeIrradianceMap = 0;
    activePrefilterMap = 0;

    // we iterate through the raw list of entities and sort them into buckets
    // this is crucial because opaque objects must be drawn first, followed by the skybox, and finally transparent
    // objects
    for (auto &e : source)
    {
        if (!e)
            continue;

        // we isolate the skybox so we can draw it at a specific time in the pipeline
        if (e->skyboxComp)
        {
            if (!skybox)
            {
                skybox = e;
                if (skybox->skyboxComp->material && skybox->skyboxComp->material->textures.count("irradiance"))
                {
                    activeIrradianceMap = skybox->skyboxComp->material->textures["irradiance"]->ID;
                }

                if (skybox->skyboxComp->material && skybox->skyboxComp->material->textures.count("prefilter"))
                {
                    activePrefilterMap = skybox->skyboxComp->material->textures["prefilter"]->ID;
                }
            }
            continue;
        }

        bool isTransparent = false;
        if (e->renderComp)
        {
            if (e->renderComp->materialOverride && e->renderComp->materialOverride->isTransparent)
                isTransparent = true;
            else
            {
                for (const auto &node : e->renderComp->nodes)
                {
                    if (node.baseMaterial && node.baseMaterial->isTransparent)
                    {
                        isTransparent = true;
                        break;
                    }
                }
            }
        }

        if (isTransparent)
            transparentEntities.push_back(e);
        else
        {
            opaqueEntities.push_back(e);
            if (e->renderComp && e->renderComp->instanceCount > 1)
                continue;

            // batching
            std::shared_ptr<Shader> shader = e->renderComp->shader;

            glm::mat4 modelMatrix = e->transform.GetModelMatrix();

            for (const auto &node : e->renderComp->nodes)
            {
                if (!node.mesh)
                    continue;

                std::shared_ptr<Material> baseMat = node.baseMaterial ? node.baseMaterial : defaultMat;
                std::shared_ptr<Material> overrideMat = e->renderComp->materialOverride;

                glm::mat4 finalMatrix = modelMatrix * node.localTransform;

                opaqueBatch[shader][baseMat][overrideMat][node.mesh.get()].push_back(finalMatrix);
            }
        }
    }
}

void Renderer::SortTransparentEntities(const glm::vec3 &camPos)
{
    // transparent objects must be sorted back-to-front based on distance to camera
    // this uses the standard painter's algorithm to ensure alpha blending works correctly
    std::sort(transparentEntities.begin(), transparentEntities.end(),
              [&camPos](const std::shared_ptr<Entity> a, const std::shared_ptr<Entity> b)
              {
                  float da = glm::dot(camPos - a->transform.position, camPos - a->transform.position);
                  float db = glm::dot(camPos - b->transform.position, camPos - b->transform.position);
                  return da > db;
              });
}

glm::vec3 Renderer::CalculateFrustumCenter(const std::vector<glm::vec4> &corners)
{
    glm::vec3 nearCenterSum = glm::vec3(0.0f);
    glm::vec3 farCenterSum = glm::vec3(0.0f);

    // the frustum corners are ordered such that the first 4 are the near plane
    for (int i = 0; i < 4; i++)
    {
        nearCenterSum += glm::vec3(corners[i]);
    }

    // and the last 4 are the far plane
    for (int i = 4; i < 8; i++)
    {
        farCenterSum += glm::vec3(corners[i]);
    }

    // by averaging the corners we find the geometric center of each plane
    glm::vec3 nearPlaneCenter = nearCenterSum / 4.0f;
    glm::vec3 farPlaneCenter = farCenterSum / 4.0f;

    // the center of the frustum is simply the midpoint between the near and far plane centers
    return (nearPlaneCenter + farPlaneCenter) / 2.0f;
}

glm::mat4 Renderer::GetLightSpaceMatrix(const float nearPlane, const float farPlane, const glm::vec3 &lightDir,
                                        const Camera &camera)
{
    // use camera.zoom for the fov, and the screen aspect ratio stored in beginscene
    const auto proj = glm::perspective(glm::radians(camera.Zoom), currentAspectRatio, nearPlane, farPlane);

    // get frustum corners in world space
    const std::vector<glm::vec4> corners = GetFrustumCornersWorldSpace(proj, viewMatrix);

    // calculate center of the frustum slice
    glm::vec3 center = glm::vec3(0, 0, 0);
    for (const auto &v : corners)
    {
        center += glm::vec3(v);
    }
    center /= corners.size();

    // position light camera
    // note: we look from (center - lightDir) towards (center)
    // this assumes lightDir is the direction the light travels
    const auto lightView = glm::lookAt(center - lightDir, center, glm::vec3(0.0f, 1.0f, 0.0f));

    // calculate bounding box
    float minX = std::numeric_limits<float>::max();
    float maxX = std::numeric_limits<float>::lowest();
    float minY = std::numeric_limits<float>::max();
    float maxY = std::numeric_limits<float>::lowest();
    float minZ = std::numeric_limits<float>::max();
    float maxZ = std::numeric_limits<float>::lowest();

    for (const auto &v : corners)
    {
        const auto trf = lightView * v;
        minX = std::min(minX, trf.x);
        maxX = std::max(maxX, trf.x);
        minY = std::min(minY, trf.y);
        maxY = std::max(maxY, trf.y);
        minZ = std::min(minZ, trf.z);
        maxZ = std::max(maxZ, trf.z);
    }

    // we expand the z bounds to ensure geometry behind the camera or far away also casts shadows
    constexpr float zPadding = 200.0f;
    minZ -= zPadding;
    maxZ += zPadding;

    const glm::mat4 lightProjection = glm::ortho(minX, maxX, minY, maxY, minZ, maxZ);
    return lightProjection * lightView;
}

std::vector<glm::mat4> Renderer::GetLightSpaceMatrices(const glm::vec3 &lightDir, const Camera &camera)
{
    std::vector<glm::mat4> ret;

    // iterate through cascade levels
    for (size_t i = 0; i < shadowCascadeLevels.size() + 1; ++i)
    {
        if (i == 0)
        {
            ret.push_back(GetLightSpaceMatrix(camera.Near, shadowCascadeLevels[i], lightDir, camera));
        }
        else if (i < shadowCascadeLevels.size())
        {
            ret.push_back(GetLightSpaceMatrix(shadowCascadeLevels[i - 1], shadowCascadeLevels[i], lightDir, camera));
        }
        else
        {
            ret.push_back(GetLightSpaceMatrix(shadowCascadeLevels[i - 1], camera.Far, lightDir, camera));
        }
    }
    return ret;
}

void Renderer::RenderShadowMap(const glm::vec3 &lightDir, const Camera &camera)
{
    // calculate matrices
    std::vector<glm::mat4> matrices = GetLightSpaceMatrices(lightDir, camera);

    // pack into struct
    ShadowData shadowData{};
    // fill Matrices
    for (size_t i = 0; i < matrices.size() && i < 16; ++i)
        shadowData.lightSpaceMatrices[i] = matrices[i];

    // fill Cascade Splits (packing floats into vec4)
    // we rely on the fact that glm::vec4 is just 4 floats in memory.
    for (size_t i = 0; i < shadowCascadeLevels.size() && i < 16; ++i)
    {
        // calculate index: 0->[0].x, 1->[0].y, 4->[1].x
        int vecIdx = i / 4;
        int compIdx = i % 4;
        shadowData.cascadePlaneDistances[vecIdx][compIdx] = shadowCascadeLevels[i];
    }

    shadowData.cascadeCount = (int) shadowCascadeLevels.size();
    shadowData.shadowFarPlane = cameraFarPlane;

    // upload to binding 2 using wrapper
    if (shadowUBO)
        shadowUBO->UploadData(&shadowData, sizeof(ShadowData));

    depthShader->use();
    shadowFBO->Bind();
    glViewport(0, 0, shadowFBO->Width(), shadowFBO->Height());
    glClear(GL_DEPTH_BUFFER_BIT);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(4.0f, 4.0f); // push depth values back slightly based on their slope

    // perform the actual draw calls for all opaque objects that should cast
    // pass identity matrices (geometry shader uses the ubo data)
    RenderBatches(depthShader);

    for (auto &e : opaqueEntities)
    {
        if (e->renderComp && e->renderComp->instanceCount > 1)
            DrawEntityInPass(e.get(), depthShader, true);
    }

    glDisable(GL_POLYGON_OFFSET_FILL);
    Framebuffer::Unbind();
    glCullFace(GL_BACK);
}

void Renderer::RenderPointShadows(const LightManager &lightManager)
{
    if (lightManager.points.empty())
        return;
    // disable culling to prevent 2D planes/open meshes from disappearing
    glDisable(GL_CULL_FACE);
    pointShadowShader->use();
    pointShadowFBO->Bind();
    glViewport(0, 0, pointShadowFBO->Width(), pointShadowFBO->Height());
    glClear(GL_DEPTH_BUFFER_BIT);

    int activeLights = std::min((int) lightManager.points.size(), 8);
    int shadowLayerIndex = 0;
    for (int i = 0; i < activeLights; i++)
    {
        if (!lightManager.points[i].enabled)
            continue;
        glm::vec3 pos = lightManager.points[i].position;
        float lightRadius = lightManager.points[i].radius;
        PointShadowData uboData;
        uboData.lightPos = glm::vec4(pos, 1.0f); // .w is padding
        uboData.farPlane = lightRadius;

        float aspect = (float) pointShadowFBO->Width() / (float) pointShadowFBO->Height();
        glm::mat4 shadowProj = glm::perspective(glm::radians(90.0f), aspect, pointShadowNear, lightRadius);

        // +X (Right)
        uboData.shadowMatrices[0] = shadowProj * glm::lookAt(pos, pos + glm::vec3(1, 0, 0), glm::vec3(0, -1, 0));
        // -X (Left)
        uboData.shadowMatrices[1] = shadowProj * glm::lookAt(pos, pos + glm::vec3(-1, 0, 0), glm::vec3(0, -1, 0));
        // +Y (Top)
        uboData.shadowMatrices[2] = shadowProj * glm::lookAt(pos, pos + glm::vec3(0, 1, 0), glm::vec3(0, 0, 1));
        // -Y (Bottom)
        uboData.shadowMatrices[3] = shadowProj * glm::lookAt(pos, pos + glm::vec3(0, -1, 0), glm::vec3(0, 0, -1));
        // +Z (Near)
        uboData.shadowMatrices[4] = shadowProj * glm::lookAt(pos, pos + glm::vec3(0, 0, 1), glm::vec3(0, -1, 0));
        // -Z (Far)
        uboData.shadowMatrices[5] = shadowProj * glm::lookAt(pos, pos + glm::vec3(0, 0, -1), glm::vec3(0, -1, 0));

        // Upload  Ubo
        pointShadowUBO->UploadData(&uboData, sizeof(PointShadowData));
        pointShadowShader->setInt("lightIndex", shadowLayerIndex);

        RenderBatches(pointShadowShader);

        for (auto &e : opaqueEntities)
        {
            if (e->renderComp && e->renderComp->instanceCount > 1)
                DrawEntityInPass(e.get(), pointShadowShader, true);
        }

        shadowLayerIndex++;
    }

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    Framebuffer::Unbind();
}

void Renderer::RenderLightingPass(std::shared_ptr<Entity> skybox, LightManager &lightManager, Framebuffer &sceneFBO,
                                  PostProcessingPipeline &postProcessor, glm::vec3 clearColor)
{
    sceneFBO.Bind();

    // restore the viewport to the full screen size since we are done with the shadow map
    glViewport(0, 0, sceneFBO.Width(), sceneFBO.Height());

    glEnable(GL_DEPTH_TEST);
    glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    // clear attachment 0 (scene) to specified sky color
    const float sceneClearColor[] = {clearColor.r, clearColor.g, clearColor.b, 1.0f};
    glClearBufferfv(GL_COLOR, 0, sceneClearColor);
    // clear attachment 1 (brightness) to black color
    const float zeroClearColor[] = {0.0f, 0.0f, 0.0f, 1.0f};
    glClearBufferfv(GL_COLOR, 1, zeroClearColor);

    // bind the shadow map texture to slot 10 so our lighting shaders can read from it
    glActiveTexture(GL_TEXTURE0 + Bindings::TEX_SLOT_CSM_SHADOW);
    glBindTexture(GL_TEXTURE_2D_ARRAY, shadowFBO->GetDepthTexture());

    // bind the shadow map texture for points lights (if any) to slot 11
    if (pointShadowFBO)
    {
        glActiveTexture(GL_TEXTURE0 + Bindings::TEX_SLOT_POINT_SHADOW);
        glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, pointShadowFBO->GetDepthTexture());
    }

    // 1. draw opaque geometry first as they write to the depth buffer
    RenderBatches();

    for (auto &e : opaqueEntities)
    {
        if (e->renderComp && e->renderComp->instanceCount > 1)
            DrawEntityInPass(e.get(), nullptr, false);
    }

    // 2. draw the skybox afterwards to optimize performance by avoiding pixels already covered by geometry
    if (skybox)
    {
        // do not write the brightness buffer attachment for skybox
        GLenum drawBuffers[] = {GL_COLOR_ATTACHMENT0};
        glDrawBuffers(1, drawBuffers);
        SubmitSkybox(*skybox->skyboxComp->mesh, skybox->skyboxComp->shader, skybox->skyboxComp->material);
        GLenum bothBuffers[] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1}; // Restore
        glDrawBuffers(2, bothBuffers);
    }

    // 3. draw debug visualizations for lights
    lightManager.RenderDebugLights(viewMatrix, projMatrix);

    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // 4. draw transparent geometry last so they blend correctly over everything else
    RenderPass(transparentEntities, viewMatrix, projMatrix, nullptr);

    glDisable(GL_BLEND);

    // 5. finally resolve any msaa and run the post-processing pipeline
    sceneFBO.ResolveToScreen();
    GLuint sceneTex = sceneFBO.GetIntermediateTexture(0);
    GLuint brightTex = sceneFBO.GetIntermediateTexture(1);
    GLuint processed = postProcessor.Apply(sceneTex, brightTex);
    postProcessor.DrawToScreen(processed);

    glEnable(GL_DEPTH_TEST);
}

void Renderer::RenderWireframePass(std::shared_ptr<Entity> skybox, LightManager &lightManager, Framebuffer &sceneFBO)
{
    // this is a simplified render path that skips post-processing and shadows
    // mostly used for debugging geometry
    glViewport(0, 0, sceneFBO.Width(), sceneFBO.Height());

    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    RenderPass(opaqueEntities, viewMatrix, projMatrix, nullptr);

    if (skybox)
    {
        SubmitSkybox(*skybox->skyboxComp->mesh, skybox->skyboxComp->shader, skybox->skyboxComp->material);
    }

    lightManager.RenderDebugLights(viewMatrix, projMatrix);
    RenderPass(transparentEntities, viewMatrix, projMatrix, nullptr);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void Renderer::RenderScene(std::vector<std::shared_ptr<Entity>> entities, Camera &camera, LightManager &lightManager,
                           Framebuffer &sceneFBO, PostProcessingPipeline &postProcessor, bool wireFrame,
                           glm::vec3 clearColor)
{
    std::shared_ptr<Entity> skyboxEntity = nullptr;

    EnsureGBuffer(sceneFBO.Width(), sceneFBO.Height());

    // organize all entities into their respective buckets for correct sorting
    CategorizeEntities(entities, skyboxEntity);

    // transparent entities need strict depth sorting to look correct
    SortTransparentEntities(camera.Position);

    // generate the shadow map texture
    glm::vec3 lightDir = glm::normalize(lightManager.GetDirectionalLightDir());
    RenderShadowMap(lightDir, camera);

    // generate the shadow map texture for point lights
    RenderPointShadows(lightManager);

    if (!wireFrame)
    {
        if (useDeferred)
        {
            RenderGeometryPass();
            RenderSSAOPass(sceneFBO.Width(), sceneFBO.Height());

            sceneFBO.Bind();
            const float clearCol[] = {clearColor.r, clearColor.g, clearColor.b, 1.0f};
            const float zeroClearColor[] = {0.0f, 0.0f, 0.0f, 1.0f};
            glClearBufferfv(GL_COLOR, 0, clearCol);
            glClearBufferfv(GL_COLOR, 1, zeroClearColor); // clear bright buffer

            RenderDeferredLightingPass(lightManager, sceneFBO, skyboxEntity);

            CopyDepthBuffer(*gBufferFBO, sceneFBO);
            sceneFBO.Bind();

            RenderLocalLightVolumes(lightManager, sceneFBO);

            // draw the objects we skipped in the G-Buffer using their own custom shaders
            for (auto &e : opaqueEntities)
            {
                if (!e || !e->renderComp)
                    continue;
                bool forceFwd = false;

                if (e->renderComp->materialOverride)
                    forceFwd = e->renderComp->materialOverride->GetBool("forceForward");
                else if (!e->renderComp->nodes.empty() && e->renderComp->nodes[0].baseMaterial)
                    forceFwd = e->renderComp->nodes[0].baseMaterial->GetBool("forceForward");

                if (forceFwd)
                    DrawEntityInPass(e.get(), nullptr, false);
            }
            if (skyboxEntity)
            {
                GLenum drawBuffers[] = {GL_COLOR_ATTACHMENT0};
                glDrawBuffers(1, drawBuffers);
                SubmitSkybox(*skyboxEntity->skyboxComp->mesh, skyboxEntity->skyboxComp->shader,
                             skyboxEntity->skyboxComp->material);
                GLenum bothBuffers[] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
                glDrawBuffers(2, bothBuffers);
            }

            lightManager.RenderDebugLights(viewMatrix, projMatrix);
            glEnable(GL_BLEND);
            glBlendEquation(GL_FUNC_ADD);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            RenderPass(transparentEntities, viewMatrix, projMatrix, nullptr);
            glDisable(GL_BLEND);

            RenderOutlinesPass(opaqueEntities);

            sceneFBO.ResolveToScreen();
            GLuint sceneTex = sceneFBO.GetIntermediateTexture(0);
            GLuint brightTex = sceneFBO.GetIntermediateTexture(1);
            GLuint processed = postProcessor.Apply(sceneTex, brightTex);
            postProcessor.DrawToScreen(processed);
        }
        else
        {
            RenderLightingPass(skyboxEntity, lightManager, sceneFBO, postProcessor, clearColor);
        }
    }
    else
    {
        RenderWireframePass(skyboxEntity, lightManager, sceneFBO);
    }
}

void Renderer::RenderGBufferImGuiWindow()
{
    if (!gBufferFBO)
        return;

    ImGui::Begin("G-Buffer Visualizer");

    float contentWidth = ImGui::GetContentRegionAvail().x;
    float aspect = (float) gBufferFBO->Height() / (float) gBufferFBO->Width();
    ImVec2 texSize = ImVec2(contentWidth, contentWidth * aspect);
    ImVec2 uv0 = ImVec2(0, 1);
    ImVec2 uv1 = ImVec2(1, 0);

    if (ImGui::CollapsingHeader("World Position (RGB) + Linear Depth (A)", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Image((void *) gBufferFBO->GetColorTexture(0), texSize, uv0, uv1);
    }

    if (ImGui::CollapsingHeader("BRDF LUT", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Image((void *) ResourceManager::GetTexture("BRDF_LUT")->ID, texSize, uv0, uv1);
    }

    if (ImGui::CollapsingHeader("World Normal (RGB) + Shininess (A)", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Image((void *) gBufferFBO->GetColorTexture(1), texSize, uv0, uv1);
    }

    if (ImGui::CollapsingHeader("Albedo (RGB) + Specular (A)", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Image((void *) gBufferFBO->GetColorTexture(2), texSize, uv0, uv1);
    }

    if (ImGui::CollapsingHeader("Emissive Map", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Image((void *) gBufferFBO->GetColorTexture(3), texSize, uv0, uv1);
    }

    ImGui::End();
}

std::vector<glm::vec4> Renderer::GetFrustumCornersWorldSpace(const glm::mat4 &proj, const glm::mat4 &view)
{
    // to find where the camera is looking in world space, we invert the view-projection matrix
    const auto inv = glm::inverse(proj * view);
    std::vector<glm::vec4> frustumCorners;

    // we define the 8 corners of the ndc cube manually for clarity
    // the z range is -1 to 1 in openGL
    const std::vector<glm::vec3> ndcCorners = {{-1, -1, -1}, {1, -1, -1}, {-1, 1, -1}, {1, 1, -1},
                                               {-1, -1, 1},  {1, -1, 1},  {-1, 1, 1},  {1, 1, 1}};

    for (const auto &ndc : ndcCorners)
    {
        // we append a w component of 1.0 to treat these as points rather than directions
        const glm::vec4 pt = inv * glm::vec4(ndc, 1.0f);

        // perspective division is required to normalize the coordinates back to cartesian space
        frustumCorners.push_back(pt / pt.w);
    }

    return frustumCorners;
}

void Renderer::ConfigureStencilForOutline(bool doOutline)
{
    // we always enable writing to the stencil buffer to create the mask
    glStencilMask(0xFF);

    if (doOutline)
    {
        // if outlines are enabled, we write a '1' to the stencil buffer for every pixel of this object
        glStencilFunc(GL_ALWAYS, 1, 0xFF);
    }
    else
    {
        // otherwise we write a '0' to clear any previous stencil values in this area
        glStencilFunc(GL_ALWAYS, 0, 0xFF);
    }
}

void Renderer::UploadMeshUniforms(const std::shared_ptr<Shader> &shader, const glm::mat4 &model)
{
    shader->use();

    // if the shader supports shadows, we verify the texture unit and bind the depth map
    if (shader->hasUniform("shadowMap"))
    {
        shader->setInt("shadowMap", Bindings::TEX_SLOT_CSM_SHADOW);
        glActiveTexture(GL_TEXTURE0 + Bindings::TEX_SLOT_CSM_SHADOW);
        glBindTexture(GL_TEXTURE_2D_ARRAY, shadowFBO->GetDepthTexture());
    }

    // if the shader supports omnidirectional shadows, we verify the texture unit and bind the depth map
    if (shader->hasUniform("pointShadowMap"))
    {
        shader->setInt("pointShadowMap", Bindings::TEX_SLOT_POINT_SHADOW);
        glActiveTexture(GL_TEXTURE0 + Bindings::TEX_SLOT_POINT_SHADOW);
        glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, pointShadowFBO->GetDepthTexture());
    }

    // upload all standard transformation matrices required for vertex processing
    if (shader->hasUniform("model"))
        shader->setMat4("model", model);
    if (shader->hasUniform("view"))
        shader->setMat4("view", viewMatrix);
    if (shader->hasUniform("projection"))
        shader->setMat4("projection", projMatrix);
    if (shader->hasUniform("viewPos"))
        shader->setVec3("viewPos", viewPosition);
}

void Renderer::RenderMeshOutline(const Mesh &mesh, const glm::mat4 &model, const glm::vec3 &color, float bloomFactor,
                                 float outlineThickness)
{
    // if the shader failed to load earlier, we simply skip the effect to avoid crashing
    if (!outlineShader)
        return;

    // we configure the stencil test to pass only where the value is NOT 1
    // effectively, this prevents us from drawing over the object itself, creating a border
    glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
    glStencilMask(0x00);

    outlineShader->use();

    // we scale the model up slightly to create the thickness of the outline
    glm::mat4 outlineModel = glm::scale(model, glm::vec3(1.0f + outlineThickness));

    outlineShader->setMat4("model", outlineModel);
    outlineShader->setMat4("view", viewMatrix);
    outlineShader->setMat4("projection", projMatrix);
    outlineShader->setVec3("color", color);
    outlineShader->setFloat("bloomFactor", bloomFactor);

    mesh.DrawSimple();

    // reset stencil state to default behavior so subsequent objects draw normally
    glStencilMask(0xFF);
    glStencilFunc(GL_ALWAYS, 0, 0xFF);
}

void Renderer::RenderMeshNormals(const Mesh &mesh, const glm::mat4 &model)
{
    if (!normalShader)
    {
        return;
    }

    // this geometry shader visualizes vertex normals as yellow lines
    // very useful for debugging lighting issues
    normalShader->use();
    normalShader->setMat4("model", model);
    normalShader->setMat4("view", viewMatrix);
    normalShader->setMat4("projection", projMatrix);

    if (normalShader->hasUniform("color"))
    {
        normalShader->setVec3("color", glm::vec3(1.0f, 1.0f, 0.0f));
    }

    mesh.DrawSimple();
}

void Renderer::RenderOutlinesPass(const std::vector<std::shared_ptr<Entity>> &entities)
{
    if (!outlineShader)
        return;
    for (auto &e : entities)
    {
        if (!e->renderComp)
            continue;
        for (const auto &node : e->renderComp->nodes)
        {
            if (!node.mesh)
                continue;
            std::shared_ptr<Material> baseMat = node.baseMaterial;
            std::shared_ptr<Material> overrideMat = e->renderComp->materialOverride;

            bool doOutline = false;
            glm::vec3 outlineCol(1.0f);
            float bloomFactor = 0.0f;
            float outlineThickness = 0.05f;

            if (overrideMat && overrideMat->bools.count("outlineEnabled"))
            {
                doOutline = overrideMat->GetBool("outlineEnabled");
                outlineCol = overrideMat->GetVec3("outlineColor");
                bloomFactor = overrideMat->GetFloat("bloomFactor");
                outlineThickness = overrideMat->GetFloat("outlineThickness", 0.05f);
            }
            else if (baseMat && baseMat->GetBool("outlineEnabled"))
            {
                doOutline = true;
                outlineCol = baseMat->GetVec3("outlineColor");
                bloomFactor = baseMat->GetFloat("bloomFactor");
                outlineThickness = baseMat->GetFloat("outlineThickness", 0.05f);
            }

            if (forceOutlines)
                doOutline = true;

            if (doOutline)
            {
                glm::mat4 nodeMatrix = e->transform.GetModelMatrix() * node.localTransform;
                RenderMeshOutline(*node.mesh, nodeMatrix, outlineCol, bloomFactor, outlineThickness);
            }
        }
    }
}

void Renderer::SubmitMesh(const glm::mat4 &model, const Mesh &mesh, const std::shared_ptr<Shader> &shader,
                          const std::shared_ptr<Material> &baseMat, const std::shared_ptr<Material> &overrideMat,
                          int instanceCount, bool isShadowPass)
{
    if (!shader)
        return;

    const Material *activeBase = baseMat ? baseMat.get() : defaultMat.get();

    activeBase->ApplyToShader(*shader, false);

    if (overrideMat)
        overrideMat->ApplyToShader(*shader, true);

    // Normals
    bool doNormals = false;
    if (overrideMat && overrideMat->bools.count("showNormals"))
        doNormals = overrideMat->GetBool("showNormals");
    else if (activeBase)
        doNormals = activeBase->GetBool("showNormals");

    // Wireframe
    bool wireframe = false;
    if (overrideMat && overrideMat->bools.count("wireframe"))
        wireframe = overrideMat->GetBool("wireframe");
    else
        wireframe = activeBase->GetBool("wireframe");

    // Outline
    bool doOutline = false;
    glm::vec3 outlineCol(1.0f);
    float bloomFactor = 0.0f;
    float outlineThickness = 0.05f;

    if (!isShadowPass)
    {
        if (overrideMat && overrideMat->bools.count("outlineEnabled"))
        {
            doOutline = overrideMat->GetBool("outlineEnabled");
            outlineCol = overrideMat->GetVec3("outlineColor");
            bloomFactor = overrideMat->GetFloat("bloomFactor");
            outlineThickness = overrideMat->GetFloat("outlineThickness", 0.05f);
        }
        else if (activeBase->GetBool("outlineEnabled"))
        {
            doOutline = true;
            outlineCol = activeBase->GetVec3("outlineColor");
            bloomFactor = activeBase->GetFloat("bloomFactor");
            outlineThickness = activeBase->GetFloat("outlineThickness", 0.05f);
        }
    }

    // Force outlines globally
    if (forceOutlines && !isShadowPass)
        doOutline = true;

    // Culling
    CullMode currentCull = activeBase->cullMode;
    if (overrideMat && overrideMat->cullMode != CullMode::Back)
        currentCull = overrideMat->cullMode;

    if (wireframe)
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    ConfigureStencilForOutline(doOutline);
    UploadMeshUniforms(shader, model);

    if (currentCull == CullMode::None)
        glDisable(GL_CULL_FACE);
    else
    {
        glEnable(GL_CULL_FACE);
        glCullFace((currentCull == CullMode::Front) ? GL_FRONT : GL_BACK);
    }

    if (instanceCount > 1)
        mesh.DrawInstanced(*shader, instanceCount);
    else
        mesh.Draw(*shader);

    if (wireframe)
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    if (doOutline && shader != gBufferShader)
        RenderMeshOutline(mesh, model, outlineCol, bloomFactor, outlineThickness);

    if (doNormals)
        RenderMeshNormals(mesh, model);

    if (currentCull != CullMode::Back)
    {
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
    }

    // cleanup texture slots
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Renderer::SubmitSkybox(const Mesh &mesh, const std::shared_ptr<Shader> &shader,
                            const std::shared_ptr<Material> &mat)
{
    if (!shader || !mat)
    {
        return;
    }

    // we need to find the specific cubemap texture from the material's texture list
    std::shared_ptr<Texture> cubeTex = nullptr;
    auto it = mat->textures.find("skybox");

    if (it != mat->textures.end())
    {
        cubeTex = it->second;
    }
    else
    {
        // fallback search for any cubemap if the specific key wasn't found
        for (const auto &kv : mat->textures)
        {
            if (kv.second && kv.second->type == TextureType::TEX_CUBEMAP)
            {
                cubeTex = kv.second;
                break;
            }
        }
    }

    if (!cubeTex)
    {
        return;
    }

    // changing the depth function to LEQUAL is a trick to render the skybox behind everything
    // even though it is technically drawn last
    glDepthFunc(GL_LEQUAL);
    glDisable(GL_CULL_FACE);

    // we strip the translation component from the view matrix so the skybox doesn't move when we walk
    glm::mat4 viewNoTrans = glm::mat4(glm::mat3(viewMatrix));

    shader->use();
    if (shader->hasUniform("view"))
        shader->setMat4("view", viewNoTrans);
    if (shader->hasUniform("projection"))
        shader->setMat4("projection", projMatrix);
    if (shader->hasUniform("skybox"))
        shader->setInt("skybox", 0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubeTex->ID);

    mesh.DrawSimple();

    // restore standard rendering state
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glDepthFunc(GL_LESS);
    glActiveTexture(GL_TEXTURE0);
}