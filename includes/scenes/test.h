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

private:
    Window& win;

    // Textures (diffuse = color, specular = shininess highlights)
    unsigned int cubeDiffuseMap, cubeSpecularMap;
    unsigned int floorDiffuseMap, floorSpecularMap;

    // The shader program for this scene
    std::shared_ptr<Shader> shader;

    Mesh cube;
    Mesh floor;

    Renderer renderer;
    LightManager lightManager;

    std::vector<Entity> entities;
};