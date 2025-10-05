#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>

#include "Helpers/camera.h"
#include "Scenes/directionalLightScene.h"
#include "Scenes/pointLightScene.h"
#include "Scenes/flashLightScene.h"
#include "Scenes/factoryScene.h"
#include "../includes/appState.h"

// Forward declarations
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);

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
    if (!glfwInit()) {
        std::cerr << "Failed to init GLFW\n";
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // -------------------------
    // 3. Create window
    // -------------------------
    GLFWwindow* window = glfwCreateWindow(appState.SCR_WIDTH, appState.SCR_HEIGHT, "Scene", NULL, NULL);
    if (!window) {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // -------------------------
    // 4. Initialize GLAD
    // -------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD\n";
        return -1;
    }

    glEnable(GL_DEPTH_TEST);

    // -------------------------
    // 5. Init scenes
    // -------------------------
    appState.scenes.push_back(new DirectionalLightScene(appState));
    appState.scenes.push_back(new PointLightScene(appState));
    appState.scenes.push_back(new FlashLightScene(appState));
    appState.scenes.push_back(new FactoryScene(appState));

    for (auto* scene : appState.scenes)
        scene->init();

    glfwSetWindowUserPointer(window, &appState);

    // -------------------------
    // 6. Register callbacks
    // -------------------------
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // -------------------------
    // 7. Main render loop
    // -------------------------
    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = static_cast<float>(glfwGetTime());
        appState.deltaTime = currentFrame - appState.lastFrame;
        appState.lastFrame = currentFrame;

        glClearColor(0.08f, 0.08f, 0.11f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glfwPollEvents();
        processInput(window);

        if (!appState.scenes.empty()) {
            appState.scenes[appState.currentSceneIndex]->update();
            appState.scenes[appState.currentSceneIndex]->render();
        }

        glfwSwapBuffers(window);
    }

    for (auto* s : appState.scenes)
        delete s;

    glfwTerminate();
    return 0;
}

// -------------------------
// Input handling
// -------------------------
void processInput(GLFWwindow* window)
{
    AppState* app = static_cast<AppState*>(glfwGetWindowUserPointer(window));
    if (!app) return;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        app->camera.ProcessKeyboard(FORWARD, app->deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        app->camera.ProcessKeyboard(BACKWARD, app->deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        app->camera.ProcessKeyboard(LEFT, app->deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        app->camera.ProcessKeyboard(RIGHT, app->deltaTime);
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    AppState* app = static_cast<AppState*>(glfwGetWindowUserPointer(window));
    if (!app) return;

    if (key == GLFW_KEY_RIGHT && action == GLFW_RELEASE) {
        app->currentSceneIndex = (app->currentSceneIndex + 1) % app->scenes.size();
        std::cout << "Switched to scene " << app->currentSceneIndex << '\n';
        glfwSetWindowTitle(window, app->scenes[app->currentSceneIndex]->name().c_str());
    }

    if (key == GLFW_KEY_LEFT && action == GLFW_RELEASE) {
        int n = (int)app->scenes.size();
        app->currentSceneIndex = (app->currentSceneIndex + n - 1) % n;
        std::cout << "Switched to scene " << app->currentSceneIndex << '\n';
        glfwSetWindowTitle(window, app->scenes[app->currentSceneIndex]->name().c_str());
    }

    if (key == GLFW_KEY_R && action == GLFW_RELEASE)
    {
        app->camera.Reset();
    }

    if (key == GLFW_KEY_F && action == GLFW_RELEASE)
    {
        if (app->wireframeEnabled)
        {
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            app->wireframeEnabled = false;
        }
        else
        {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            app->wireframeEnabled = true;
        }
    }

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        if (app->mouseCaptured) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            app->mouseCaptured = false;
            std::cout << "Mouse released\n";
        }
        else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            app->mouseCaptured = true;
            app->firstMouse = true;
            std::cout << "Mouse captured\n";
        }
    }
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    AppState* app = static_cast<AppState*>(glfwGetWindowUserPointer(window));
    if (!app) return;

    app->SCR_WIDTH = width;
    app->SCR_HEIGHT = height;
    glViewport(0, 0, width, height);
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    AppState* app = static_cast<AppState*>(glfwGetWindowUserPointer(window));
    if (!app) return;

    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS && !app->mouseCaptured) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        app->mouseCaptured = true;
        app->firstMouse = true;
    }
}

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    AppState* app = static_cast<AppState*>(glfwGetWindowUserPointer(window));
    if (!app || !app->mouseCaptured) return;

    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (app->firstMouse) {
        app->lastX = xpos;
        app->lastY = ypos;
        app->firstMouse = false;
    }

    float xoffset = xpos - app->lastX;
    float yoffset = app->lastY - ypos;

    app->lastX = xpos;
    app->lastY = ypos;

    app->camera.ProcessMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    AppState* app = static_cast<AppState*>(glfwGetWindowUserPointer(window));
    if (app)
        app->camera.ProcessMouseScroll((float)yoffset);
}