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
    bool mouseCaptured = true;
    bool firstMouse = true;
    bool wireframeEnabled = false;
    unsigned int SCR_WIDTH = 800;
    unsigned int SCR_HEIGHT = 600;
    float lastX = SCR_WIDTH / 2.0f;
    float lastY = SCR_HEIGHT / 2.0f;

    float deltaTime = 0.0f;
    float lastFrame = 0.0f;
};