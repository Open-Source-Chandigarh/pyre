#pragma once
#include <vector>
#include <memory>
#include "helpers/camera.h"
#include "core/Entity.h"

class Scene;
class Renderer;
class LightManager;
class Window; 

struct AppState 
{
    AppState();
    ~AppState();

    std::unique_ptr<Renderer> renderer;
    std::unique_ptr<LightManager> lightManager;

    std::vector<std::unique_ptr<Scene>> scenes;
    int currentSceneIndex = 0;
    std::shared_ptr<Entity> selectedEntity = nullptr;
    
    Camera camera = Camera(glm::vec3(0.0f, 0.0f, 3.0f));
    int width = 800;
    int height = 600;
    
    bool wireframeEnabled = false;
    float deltaTime = 0.0f;
    float lastFrame = 0.0f;

    float GetAspectRatio() const {
        return (height > 0) ? (float)width / (float)height : 1.0f;
    }
};