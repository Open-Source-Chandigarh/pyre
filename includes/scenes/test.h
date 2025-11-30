#pragma once
#include "scenes/scene.h"
#include "core/Window.h"
#include "core/LightManager.h"
#include "core/rendering/Renderer.h"
#include "core/Entity.h"


class Test : public Scene
{
public:
    Test(Window& win);

    // called once when the scene is loaded
    virtual void init();

    // called every frame
    virtual void update();

    // called every frame after update
    virtual void render();

    // optional: scene name
    virtual std::string name() const { return "Test Scene"; };

    virtual void OnResize(int w, int h) override {}

private:
    Window& win;

    // Textures (diffuse = color, specular = shininess highlights)
    std::shared_ptr<Texture> cubeDiffuseMap, cubeSpecularMap;
    std::shared_ptr<Texture> floorDiffuseMap, floorSpecularMap;

    // The shader program for this scene
    std::shared_ptr<Shader> shader;

    Mesh cube;
    Mesh plane;
    Mesh skyMesh;

    Renderer renderer;
    LightManager lightManager;

    std::vector<std::shared_ptr<Entity>> entities;
};