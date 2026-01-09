#pragma once
#include <string>
#include <memory>
#include "core/Window.h"
#include "core/Entity.h"

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
    virtual void Render(AppState &appState); 
    
    virtual std::string Name() const = 0;
    virtual void OnResize(int w, int h);
    glm::vec3 clearColor = glm::vec3(0.1f, 0.1f, 0.1f);
    
protected:
    std::string name;
    Window *win = nullptr;
    std::vector<std::shared_ptr<Entity>> entities;
    std::unique_ptr<Framebuffer> sceneFBO;
    std::unique_ptr<PostProcessingPipeline> postPipeline;
};