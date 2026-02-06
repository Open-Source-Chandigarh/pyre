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

    std::vector<std::shared_ptr<Entity>>& GetEntities() { return entities; }
    PostProcessingPipeline* GetPostPipeline() { return postPipeline.get(); }
    
    // Entity management for serialization
    std::shared_ptr<Entity> FindEntityByName(const std::string& name)
    {
        for (auto& e : entities)
        {
            if (e && e->name == name) return e;
        }
        return nullptr;
    }
    
    std::shared_ptr<Entity> CreateEntity(const std::string& name = "Entity")
    {
        auto e = Entity::Create(name);
        entities.push_back(e);
        return e;
    }
    
    void ClearEntities() { entities.clear(); }
    
protected:
    std::string name;
    Window *win = nullptr;
    std::vector<std::shared_ptr<Entity>> entities;
    std::unique_ptr<Framebuffer> sceneFBO;
    std::unique_ptr<PostProcessingPipeline> postPipeline;
};