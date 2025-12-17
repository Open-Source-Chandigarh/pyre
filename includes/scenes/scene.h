#pragma once
#include <string>
#include <memory>
#include "core/Window.h"

// Forward declarations
class AppState;
class Framebuffer;
class PostProcessingPipeline;

class Scene {
public:
    Scene();
    virtual ~Scene();

    void BindWindow(Window *win);
    virtual void Init(AppState &appState) = 0;
    virtual void OnActivate(AppState &appState) = 0;
    virtual void Update(AppState &appState) = 0;
    virtual void Render(AppState &appState) = 0; 
    
    virtual std::string Name() const = 0;
    virtual void OnResize(int w, int h);

protected:
    Window *win = nullptr;
    std::unique_ptr<Framebuffer> sceneFBO;
    std::unique_ptr<PostProcessingPipeline> postPipeline;
};