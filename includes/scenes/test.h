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
    Test(Window& win);

    // called once when the scene is loaded
    virtual void init();

    // called every frame
    virtual void update();

    // called every frame after update
    virtual void render();

    // optional: scene name
    virtual std::string name() const { return "Test Scene"; };

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

    Mesh cube;
    Mesh plane;
    Mesh skyMesh;

    std::shared_ptr<Renderer> renderer;
    std::shared_ptr<LightManager> lightManager;

    std::vector<std::shared_ptr<Entity>> entities;
};