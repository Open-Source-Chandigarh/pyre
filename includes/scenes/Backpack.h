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

class Backpack : public Scene
{
  public:
    Backpack();
    ~Backpack();

    // Called once when the scene is created (setup VAOs, VBOs, shaders, textures, etc.)
    void Init(AppState &appState) override;

    // Called every frame to update logic (e.g., rotations, animations)
    void Update(AppState &appState) override;

    // Called when this scene is activated
    void OnActivate(AppState &appState) override;

    std::string Name() const override
    {
        return "Model Loading Demo Scene";
    }

  private:
    std::shared_ptr<Shader> shader;
    std::shared_ptr<Model> obj;
    std::shared_ptr<Mesh> skyMesh;

    float rotationAngle;
    float rotationSpeed;
};