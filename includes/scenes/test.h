#pragma once
#include "scenes/scene.h"
#include "core/Window.h"
#include "core/LightManager.h"
#include "core/rendering/Renderer.h"
#include "core/Entity.h"
#include "core/rendering/Framebuffer.h"
#include "core/postprocessing/PostProcessingPipeline.h"

class Test : public Scene
{
public:
    Test();
    ~Test();

    // called once when the scene is created (setup VAOs, VBOs, shaders, textures, etc.)
    void Init(AppState &appState) override;

    // called every frame to update logic (e.g., rotations, animations)
    void Update(AppState &appState) override;

    // Called when this scene is activated
    void OnActivate(AppState &appState) override;

    virtual std::string Name() const { return "Test Scene"; };

private:
    std::shared_ptr<Shader> shader;
    std::shared_ptr<Mesh> cube;
    std::shared_ptr<Mesh> plane;
    std::shared_ptr<Mesh> skyMesh;
};