#include <thirdparty/glad/glad.h>
#include <thirdparty/GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include "helpers/camera.h"
#include "scenes/factoryScene.h"
#include "scenes/Space.h"
#include "scenes/ToonScene.h"
#include "scenes/backpack.h"
#include "state/appState.h"
#include "core/Window.h"
#include "core/InputManager.h"
#include "core/rendering/Model.h"
#include "scenes/test.h"

void SetupInput(Window &win);

int main()
{
   
    AppState appState;
    appState.camera = Camera(glm::vec3(0.0f, 0.0f, 3.0f));

    
    Window win(800, 600, "win");
    win.SetAppState(&appState);


    appState.scenes.push_back(new FactoryScene(win));
    appState.scenes.push_back(new Backpack(win));
    appState.scenes.push_back(new Test(win));
    appState.scenes.push_back(new ToonScene(win));
    appState.scenes.push_back(new Space(win));

    for (auto* scene : appState.scenes)
        scene->init();

   

    SetupInput(win);

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);

    while (!win.ShouldClose())
    {
        float currentFrame = static_cast<float>(glfwGetTime());
        appState.deltaTime = currentFrame - appState.lastFrame;
        appState.lastFrame = currentFrame;

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);


        win.GetInputManager()->Update(appState.deltaTime);

        if (!appState.scenes.empty()) {
            appState.scenes[appState.currentSceneIndex]->update();
            appState.scenes[appState.currentSceneIndex]->render();
        }

        glStencilMask(0xFF);
        glStencilFunc(GL_ALWAYS, 0, 0xFF);
        win.PollEvents();
        win.SwapBuffers();
    }

    for (auto* s : appState.scenes)
        delete s;

    glfwTerminate();
    return 0;
}

void SetupInput(Window &win)
{
    InputManager* input = win.GetInputManager();
    AppState* appState = win.GetAppState();
    
    // Continuous movement
    input->BindKeyContinuous(GLFW_KEY_W, [appState](float dt) { 
        appState->camera.ProcessKeyboard(FORWARD, dt); 
    });
    input->BindKeyContinuous(GLFW_KEY_S, [appState](float dt) { 
        appState->camera.ProcessKeyboard(BACKWARD, dt); 
    });
    input->BindKeyContinuous(GLFW_KEY_A, [appState](float dt) { 
        appState->camera.ProcessKeyboard(LEFT, dt); 
    });
    input->BindKeyContinuous(GLFW_KEY_D, [appState](float dt) { 
        appState->camera.ProcessKeyboard(RIGHT, dt); 
    });

    // Mouse movement
    input->BindMouseMove([appState](double xoffset, double yoffset) {
        appState->camera.ProcessMouseMovement((float)xoffset, (float)yoffset);
    });

    // Scroll
    input->BindScroll([appState](double yoffset) {
        appState->camera.ProcessMouseScroll((float)yoffset);
    });

    input->BindKeyEvent(GLFW_KEY_F, GLFW_RELEASE, [appState]() {
        if (appState->wireframeEnabled) { 
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); 
            appState->wireframeEnabled = false; 
        }
        else { 
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); 
            appState->wireframeEnabled = true; 
        }
    });

    input->BindKeyEvent(GLFW_KEY_R, GLFW_RELEASE, [appState]() {
        appState->camera.Reset();
    });

    GLFWwindow* nativeWin = win.GetNative();

    input->BindKeyEvent(GLFW_KEY_RIGHT, GLFW_RELEASE, [appState, nativeWin]() {
        appState->currentSceneIndex = (appState->currentSceneIndex + 1) % appState->scenes.size();
        if (!appState->scenes.empty())
            glfwSetWindowTitle(nativeWin, appState->scenes[appState->currentSceneIndex]->name().c_str());
    });

    input->BindKeyEvent(GLFW_KEY_LEFT, GLFW_RELEASE, [appState, nativeWin]() {
        int n = (int)appState->scenes.size();
        appState->currentSceneIndex = (appState->currentSceneIndex + n - 1) % n;
        if (!appState->scenes.empty())
            glfwSetWindowTitle(nativeWin, appState->scenes[appState->currentSceneIndex]->name().c_str());
    });

    input->BindKeyEvent(GLFW_KEY_ESCAPE, GLFW_PRESS, [input]() {
        input->ToggleMouseCapture();
    });
}