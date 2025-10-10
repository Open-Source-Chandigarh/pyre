#pragma once
#include <thirdparty/glm/glm.hpp>
#include <vector>
#include "Scene.h"
#include "helpers/shaderClass.h"
#include "helpers/camera.h"
#include "core/Window.h"
#include "state/appState.h"
#include "core/rendering/Mesh.h"
#include "core/rendering/Renderer.h"
#include "core/LightManager.h"
#include "core/Entity.h"
#include "core/rendering/Model.h"


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

private:
    Window& win;

    // The shader program for this scene
    std::shared_ptr<Shader> shader;

    Renderer renderer;
    LightManager lightManager;

    std::vector<Entity> entities;

    Model obj;

    // Animation control for rotations
    float rotationAngle;
    float rotationSpeed;
};