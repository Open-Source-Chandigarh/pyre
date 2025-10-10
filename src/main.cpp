#include <thirdparty/glad/glad.h>
#include <thirdparty/GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include "Helpers/camera.h"
//#include "Scenes/directionalLightScene.h"
//#include "Scenes/pointLightScene.h"
//#include "Scenes/flashLightScene.h"
#include "scenes/factoryScene.h"
#include "scenes/backpack.h"
#include "state/appState.h"
#include "core/Window.h"
#include "core/InputManager.h"
#include "core/rendering/Model.h"

int main()
{
    // -------------------------
    // 1. Initialize AppState
    // -------------------------
    AppState appState;
    appState.camera = Camera(glm::vec3(0.0f, 0.0f, 3.0f));

    // -------------------------
    // 2. Initialize GLFW
    // -------------------------
    Window win(800, 600, "win");
    win.SetAppState(&appState);

    // -------------------------
    // 5. Init scenes
    // -------------------------
    appState.scenes.push_back(new FactoryScene(win));
    appState.scenes.push_back(new Backpack(win));

    for (auto* scene : appState.scenes)
        scene->init();


    // 3. Bind inputs
    InputManager* input = win.GetInputManager();

    // Continuous movement (bind to keys; callbacks receive dt)
    input->BindKeyContinuous(GLFW_KEY_W, [&](float dt) { appState.camera.ProcessKeyboard(FORWARD, dt); });
    input->BindKeyContinuous(GLFW_KEY_S, [&](float dt) { appState.camera.ProcessKeyboard(BACKWARD, dt); });
    input->BindKeyContinuous(GLFW_KEY_A, [&](float dt) { appState.camera.ProcessKeyboard(LEFT, dt); });
    input->BindKeyContinuous(GLFW_KEY_D, [&](float dt) { appState.camera.ProcessKeyboard(RIGHT, dt); });

    // Mouse movement -> camera rotation (mouse callback gives offsets)
    input->BindMouseMove([&](double xoffset, double yoffset) {
            appState.camera.ProcessMouseMovement((float)xoffset, (float)yoffset);
        });

    // Scroll -> camera zoom
    input->BindScroll([&](double yoffset) {
        appState.camera.ProcessMouseScroll((float)yoffset);
        });

    // Event bindings
    input->BindKeyEvent(GLFW_KEY_F, GLFW_RELEASE, [&]() {
        if (appState.wireframeEnabled) { glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); appState.wireframeEnabled = false; }
        else { glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); appState.wireframeEnabled = true; }
     });

    input->BindKeyEvent(GLFW_KEY_R, GLFW_RELEASE, [&]() {
        appState.camera.Reset();
        });

    // Scene switching (event)
    input->BindKeyEvent(GLFW_KEY_RIGHT, GLFW_RELEASE, [&]() {
        appState.currentSceneIndex = (appState.currentSceneIndex + 1) % appState.scenes.size();
        if (!appState.scenes.empty())
            glfwSetWindowTitle(win.GetNative(), appState.scenes[appState.currentSceneIndex]->name().c_str());
        });

    input->BindKeyEvent(GLFW_KEY_LEFT, GLFW_RELEASE, [&]() {
        int n = (int)appState.scenes.size();
        appState.currentSceneIndex = (appState.currentSceneIndex + n - 1) % n;
        if (!appState.scenes.empty())
            glfwSetWindowTitle(win.GetNative(), appState.scenes[appState.currentSceneIndex]->name().c_str());
        });

    // Toggle mouse capture (press ESC)
    input->BindKeyEvent(GLFW_KEY_ESCAPE, GLFW_PRESS, [&]() {
        input->ToggleMouseCapture();
    });

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // -------------------------
    // 7. Main render loop
    // -------------------------
    while (!win.ShouldClose())
    {
        float currentFrame = static_cast<float>(glfwGetTime());
        appState.deltaTime = currentFrame - appState.lastFrame;
        appState.lastFrame = currentFrame;

        glClearColor(0.08f, 0.08f, 0.11f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        win.PollEvents();
        input->Update(appState.deltaTime);

        if (!appState.scenes.empty()) {
            appState.scenes[appState.currentSceneIndex]->update();
            appState.scenes[appState.currentSceneIndex]->render();
        }

        win.SwapBuffers();
    }

    for (auto* s : appState.scenes)
        delete s;

    glfwTerminate();
    return 0;
}