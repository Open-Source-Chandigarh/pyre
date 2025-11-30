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

class Space : public Scene {
public:
    Space(Window& win);
    ~Space() = default;

    // Called once when the scene is created (setup VAOs, VBOs, shaders, textures, etc.)
    void init() override;

    // Called every frame to update logic (e.g., rotations, animations)
    void update() override;

    // Called every frame to draw objects
    void render() override;

    // Scene display name (helpful when switching scenes)
    std::string name() const override { return "Space Scene"; }

    virtual void OnResize(int w, int h) override {}

private:
    Window& win;

    // The shader program for this scene
    std::shared_ptr<Shader> planetShader;
    std::shared_ptr<Shader> asteroidShader;

    Renderer renderer;
    LightManager lightManager;

    std::vector<std::shared_ptr<Entity>> entities;

    std::shared_ptr<Model> planet;
    std::shared_ptr<Model> asteroid;

    Mesh skyMesh;

    std::vector<glm::mat4> asteroidTransforms;

    // Animation control for rotations
    float rotationAngle;
    float rotationSpeed;
};