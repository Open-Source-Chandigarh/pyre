#pragma once
#include <vector>
#include "../src/Helpers/camera.h"
#include "../src/Scenes/scene.h"
// -------------------------
// Struct for all shared app state
// -------------------------
struct AppState {
    std::vector<Scene*> scenes;
    int currentSceneIndex = 0;
    Camera camera = Camera(glm::vec3(0.0f, 0.0f, 3.0f));
    bool wireframeEnabled = false;
    float deltaTime = 0.0f;
    float lastFrame = 0.0f;
};