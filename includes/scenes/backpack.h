#pragma once
#include <thirdparty/glm/glm.hpp>
#include <vector>
#include <memory>
#include "Scene.h"
#include "helpers/Shader.h"
#include "helpers/camera.h"
#include "core/Window.h"
#include "state/appState.h"
#include "core/rendering/Mesh.h"
#include "core/rendering/Renderer.h"
#include "core/LightManager.h"
#include "core/Entity.h"
#include "core/rendering/Model.h"
#include "core/rendering/Framebuffer.h"
#include "core/postprocessing/PostProcessingPipeline.h"


// This class represents a Scene that demonstrates lighting with all the light types combined (directional, point, spot).
// It derives from the base Scene class, so it must implement init(), update(), render(), and name().
class Backpack : public Scene {
public:
    Backpack(Window& win);
    ~Backpack();

    // Called once when the scene is created (setup VAOs, VBOs, shaders, textures, etc.)
    void init() override;

    // Called every frame to update logic (e.g., rotations, animations)
    void update() override;

    // Called every frame to draw objects
    void render() override;

    // Scene display name (helpful when switching scenes)
    std::string name() const override { return "Model Loading Demo Scene"; }

    virtual void OnResize(int w, int h) override { 
        if (sceneFBO) sceneFBO->Resize((unsigned int)w, (unsigned int)h);
        if (postPipeline) postPipeline->Resize((unsigned int)w, (unsigned int)h);
    }
private:
    std::shared_ptr<Framebuffer> sceneFBO;
    std::shared_ptr<PostProcessingPipeline> postPipeline;
    Window& win;

    // The shader program for this scene
    std::shared_ptr<Shader> shader;

    std::shared_ptr<Renderer> renderer;
    std::shared_ptr<LightManager> lightManager;

    std::vector<std::shared_ptr<Entity>> entities;

    std::shared_ptr<Model> obj;

    // Animation control for rotations
    float rotationAngle;
    float rotationSpeed;
};