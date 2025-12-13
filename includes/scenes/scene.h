#pragma once
#include <string>
#include <memory>
#include "core/Window.h"
#include "core/rendering/Renderer.h"
#include "core/LightManager.h"
#include "core/rendering/Framebuffer.h"
#include "core/postprocessing/PostProcessingPipeline.h"

class Scene {
public:
    Scene(Window& win) : win(win) {
        renderer = std::make_shared<Renderer>();
        lightManager = std::make_shared<LightManager>();
        
        sceneFBO = std::make_shared<Framebuffer>(
            (unsigned int)win.Width(), (unsigned int)win.Height(), true, true);
            
        postPipeline = std::make_shared<PostProcessingPipeline>(
            (unsigned int)win.Width(), (unsigned int)win.Height());
        
        postPipeline->AddGammaCorrection(2.2f);
    }

    virtual ~Scene() = default;

    virtual void init() = 0;
    virtual void update() = 0;
    virtual void render() = 0;
    virtual std::string name() const = 0;

    virtual void OnResize(int w, int h) {
        if (sceneFBO) sceneFBO->Resize((unsigned int)w, (unsigned int)h);
        if (postPipeline) postPipeline->Resize((unsigned int)w, (unsigned int)h);
    }

protected:
    Window& win;
    std::shared_ptr<Renderer> renderer;
    std::shared_ptr<LightManager> lightManager;
    std::shared_ptr<Framebuffer> sceneFBO;
    std::shared_ptr<PostProcessingPipeline> postPipeline;
};