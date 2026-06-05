#pragma once
#include "Scene.h"
#include "application/appState.h"
#include "core/Entity.h"
#include "core/LightManager.h"
#include "core/Window.h"
#include "core/postprocessing/PostProcessingPipeline.h"
#include "core/rendering/Framebuffer.h"
#include "core/rendering/Mesh.h"
#include "core/rendering/Model.h"
#include "core/rendering/Renderer.h"
#include "helpers/Shader.h"
#include "helpers/camera.h"
#include <memory>
#include <thirdparty/glm/glm.hpp>
#include <vector>

class Space : public Scene
{
  public:
    Space();
    ~Space();

    // Called once when the scene is created (setup VAOs, VBOs, shaders, textures, etc.)
    void Init(AppState &appState) override;

    // Called every frame to update logic (e.g., rotations, animations)
    void Update(AppState &appState) override;

    // Called when this scene is activated
    void OnActivate(AppState &appState) override;

    // Scene display name (helpful when switching scenes)
    std::string Name() const override
    {
        return "Space Scene";
    }

  private:
    // The shader program for this scene
    std::shared_ptr<Shader> planetShader;
    std::shared_ptr<Shader> asteroidShader;

    std::shared_ptr<Model> planet;
    std::shared_ptr<Model> asteroid;

    std::shared_ptr<Mesh> skyMesh;

    std::vector<glm::mat4> asteroidTransforms;

    // Animation control for rotations
    float rotationAngle;
    float rotationSpeed;
};