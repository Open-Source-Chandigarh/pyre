#pragma once
#include "scenes/scene.h"
#include "core/Window.h"
#include "core/LightManager.h"
#include "core/rendering/Renderer.h"
#include "core/Entity.h"


class ToonScene : public Scene
{
public:
    ToonScene(Window& win);

    // called once when the scene is loaded
    virtual void init();

    // called every frame
    virtual void update();

    // called every frame after update
    virtual void render();

    // optional: scene name
    virtual std::string name() const { return "Toon Scene"; };

    virtual void OnResize(int w, int h) override {}

private:
    Window& win;

    std::shared_ptr<Shader> toonShader;

    Mesh cube;
    Mesh sphere;
    Mesh torus;

    Renderer renderer;
    LightManager lightManager;

    std::vector<std::shared_ptr<Entity>> entities;
};