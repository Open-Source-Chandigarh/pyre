#pragma once
#include <thirdparty/glm/glm.hpp>
#include <vector>
#include <memory>
#include "Scene.h"
#include "helpers/Shader.h"
#include "helpers/camera.h"
#include "core/Window.h"
#include "application/appState.h"
#include "core/rendering/Mesh.h"
#include "core/rendering/Renderer.h"
#include "core/LightManager.h"
#include "core/Entity.h"
#include "core/rendering/Model.h"
#include "core/rendering/Framebuffer.h"
#include "core/postprocessing/PostProcessingPipeline.h"

class Backpack : public Scene {
public:
    Backpack();
    ~Backpack();

    // Called once when the scene is created (setup VAOs, VBOs, shaders, textures, etc.)
    void Init(AppState &appState) override;

    // Called every frame to update logic (e.g., rotations, animations)
    void Update(AppState &appState) override;

    // Called when this scene is activated
    void OnActivate(AppState &appState) override;

    // Called every frame to draw objects
    void Render(AppState &appState) override;

    std::string Name() const override { return "Model Loading Demo Scene"; }

private:
    std::shared_ptr<Shader> shader;
    std::vector<std::shared_ptr<Entity>> entities;
    std::shared_ptr<Model> obj;

    float rotationAngle;
    float rotationSpeed;
};