#pragma once
#include "core/LightManager.h"
#include "core/UniformBuffer.h"
#include "core/rendering/Framebuffer.h"
#include "core/rendering/Mesh.h"
#include "helpers/Shader.h"
#include <glm/glm.hpp>
#include <memory>
#include <random>
#include <unordered_map>
#include <vector>

class Model;
class Camera;
class PostProcessingPipeline;
struct Entity;

class Renderer
{
  public:
    Renderer();
    ~Renderer();

    void BeginScene(const Camera &camera, LightManager &lightManager, float aspectRatio);

    void RenderScene(std::vector<std::shared_ptr<Entity>> entities, Camera &camera, LightManager &lightManager,
                     Framebuffer &sceneFBO, PostProcessingPipeline &postProcessor, bool wireFrame = false,
                     glm::vec3 clearColor = glm::vec3(0.1f));

    // submit mesh
    // handles both single meshes and instanced meshes
    void SubmitMesh(const glm::mat4 &model, const Mesh &mesh, const std::shared_ptr<Shader> &shader,
                    const std::shared_ptr<Material> &baseMat, const std::shared_ptr<Material> &overrideMat,
                    int instanceCount = 1, bool isShadowPass = false);

    void SubmitSkybox(const Mesh &mesh, const std::shared_ptr<Shader> &shader, const std::shared_ptr<Material> &mat);

    // debug gbuffer
    void RenderGBufferImGuiWindow();
    void RenderQuad();

    void EndScene();

    std::shared_ptr<Shader> outlineShader;
    std::shared_ptr<Shader> normalShader;
    bool forceOutlines = false;
    bool useDeferred = false;
    float ssaoRadius = 0.5f;
    float iblStrength = 1.0f;

  private:
    // internal initialization helpers

    // calculates view and projection matrices based on camera state
    void SetupCameraGlobals(const Camera &camera, float aspectRatio);

    // resets openGL state for a fresh frame (depth, stencil, clearing)
    void ResetGlState();

    // ensures internal shaders (outline, depth, normal) are loaded
    void LoadRequiredShaders();

    // creates the shadow framebuffer for dir lights if it doesn't exist yet
    void EnsureShadowBuffer();

    void EnsureSSAOBuffer(unsigned int width, unsigned int height);

    // creates the shadow framebuffer for point lights if it doesn't exist yet
    void EnsurePointShadowBuffer();

    void EnsureGBuffer(unsigned int width, unsigned int height);

    void InitSSAO();

    // creates the uniform buffer for csm shadow matrices
    void CreateShadowUBO();

    // creates the uniform buffer for point shadow matrices
    void CreatePointShadowUBO();

    // render pass logic

    // render opaque objects to gbuffer
    void RenderGeometryPass();

    // render ssao pass
    void RenderSSAOPass(unsigned int width, unsigned int height);

    // helper to draw a single entity based on its components (mesh vs model)
    void DrawEntityInPass(Entity *e, std::shared_ptr<Shader> shaderOverride, bool isShadowPass = false);

    // generic helper to draw a list of entities with a specific view/proj matrix
    void RenderPass(const std::vector<std::shared_ptr<Entity>> &entities, const glm::mat4 &view, const glm::mat4 &proj,
                    std::shared_ptr<Shader> shaderOverride = nullptr, bool isShadowPass = false);

    void RenderBatches(const std::shared_ptr<Shader> &overrideShader = nullptr);

    void CopyDepthBuffer(Framebuffer &source, Framebuffer &dest);

    // scene organization helpers

    // sorts entities into opaque, transparent, and skybox buckets
    void CategorizeEntities(const std::vector<std::shared_ptr<Entity>> &source, std::shared_ptr<Entity> &skybox);

    // sorts transparent objects back-to-front for correct alpha blending
    void SortTransparentEntities(const glm::vec3 &camPos);

    // render outlines for deferred shading
    void RenderOutlinesPass(const std::vector<std::shared_ptr<Entity>> &entities);

    // shadow calculation math

    // finds the geometric center of the view frustum
    glm::vec3 CalculateFrustumCenter(const std::vector<glm::vec4> &corners);

    // calculates the view-projection matrix for the directional light shadow camera
    glm::mat4 GetLightSpaceMatrix(const float nearPlane, const float farPlane, const glm::vec3 &lightDir,
                                  const Camera &camera);

    std::vector<glm::mat4> GetLightSpaceMatrices(const glm::vec3 &lightDir, const Camera &camera);

    // renders the depth map for shadows (dir, point, spot etc..)
    void RenderShadowMap(const glm::vec3 &lightDir, const Camera &camera);

    // renders the depth map for point shadows (used inside RenderShadowMap func)
    void RenderPointShadows(const LightManager &lightManager);

    // main lighting passes

    void RenderDeferredLightingPass(LightManager &lightManager, Framebuffer &sceneFBO, std::shared_ptr<Entity> skybox);

    void RenderLocalLightVolumes(LightManager &lightManager, Framebuffer &sceneFBO);

    // forward pass that renders color, lighting, skybox, and transparent objects
    void RenderLightingPass(std::shared_ptr<Entity> skybox, LightManager &lightManager, Framebuffer &sceneFBO,
                            PostProcessingPipeline &postProcessor, glm::vec3 clearColor);

    // simplified debug pass for wireframe mode
    void RenderWireframePass(std::shared_ptr<Entity> skybox, LightManager &lightManager, Framebuffer &sceneFBO);

    // low level submission helpers

    // gets the camera's view frustum corners in world space coordinates
    std::vector<glm::vec4> GetFrustumCornersWorldSpace(const glm::mat4 &proj, const glm::mat4 &view);

    // sets up stencil mask for outline effects
    void ConfigureStencilForOutline(bool doOutline);

    // sends standard matrices (model, view, proj, shadow) to the shader
    void UploadMeshUniforms(const std::shared_ptr<Shader> &shader, const glm::mat4 &model);

    // draws the slightly scaled outline mesh if enabled
    void RenderMeshOutline(const Mesh &mesh, const glm::mat4 &model, const glm::vec3 &color, float bloomFactor,
                           float outlineThickness);

    // draws debug lines for vertex normals if enabled
    void RenderMeshNormals(const Mesh &mesh, const glm::mat4 &model);

    void InitQuad();

    // member variables

    // dynamic batching
    // this group matrices by Shader -> Base Material -> Override Material -> Mesh -> Matrices
    std::unordered_map<std::shared_ptr<Shader>,
                       std::unordered_map<std::shared_ptr<Material>,
                                          std::unordered_map<std::shared_ptr<Material>,
                                                             std::unordered_map<Mesh *, std::vector<glm::mat4>>>>>
        opaqueBatch;

    std::vector<std::shared_ptr<Entity>> transparentEntities;
    std::vector<std::shared_ptr<Entity>> opaqueEntities;

    // The fallback material for objects that don't have one
    std::shared_ptr<Material> defaultMat;
    glm::mat4 viewMatrix;
    glm::mat4 projMatrix;
    glm::vec3 viewPosition;
    float currentAspectRatio;
    float cameraFarPlane;
    float cameraNearPlane;
    std::unique_ptr<UniformBuffer> cameraUBO; // binding 0

    // g-buffer
    std::unique_ptr<Framebuffer> gBufferFBO;
    std::shared_ptr<Shader> gBufferShader;
    std::shared_ptr<Shader> deferredLightingShader;
    std::shared_ptr<Mesh> pointLightSphere;
    std::shared_ptr<Shader> pointLightVolumeShader;
    GLuint quadVAO = 0;
    GLuint quadVBO = 0;

    // ibl
    GLuint activeIrradianceMap = 0;
    GLuint activePrefilterMap = 0;

    // ssao
    GLuint noiseTexture = 0;
    std::unique_ptr<Framebuffer> ssaoFBO;
    std::unique_ptr<Framebuffer> ssaoBlurFBO;
    std::shared_ptr<Shader> ssaoShader;
    std::shared_ptr<Shader> ssaoBlurShader;
    std::vector<glm::vec3> ssaoKernel;

    // shadow mapping resources

    // dir light shadows (CSM)
    std::unique_ptr<UniformBuffer> shadowUBO; // binding 2
    std::unique_ptr<Framebuffer> shadowFBO;
    std::shared_ptr<Shader> depthShader;
    glm::mat4 lightSpaceMatrix;
    // 3 splits = 4 generic shadow maps (near, mid, far, veryFar)
    std::vector<float> shadowCascadeLevels = {30.0f, 100.0f, 250.0f};

    // spot light shadows (omni)
    std::unique_ptr<Framebuffer> pointShadowFBO;
    std::unique_ptr<UniformBuffer> pointShadowUBO; // binding 3
    std::shared_ptr<Shader> pointShadowShader;
    float pointShadowNear = 0.1f;
    float pointShadowFar = 25.0f;
};