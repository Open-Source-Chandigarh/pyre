// main.cpp
// -------------------------
// Entry point of the application.
// Manages window creation, OpenGL initialization, 
// scene switching, and input handling.
//
// NOTE for contributors:
// - Each "Scene" (like DirectionalLightScene, DiffuseAndSpecularMapScene)
//   handles its own setup (shaders, models, uniforms, etc).
// - main.cpp is only responsible for high-level orchestration
//   (switching scenes, handling inputs, maintaining timing, etc).
// -------------------------

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>

#include "Helpers/camera.h"
#include "Scenes/directionalLightScene.h"
#include "Scenes/diffuseAndSpecularMapScene.h"

// -------------------------
// Global settings
// -------------------------
unsigned int SCR_WIDTH = 800;
unsigned int SCR_HEIGHT = 600;

// Global camera object
// NOTE: Camera is shared across all scenes
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;   // tracks if mouse moved for the first time
bool mouseCaptured = true; // controls mouse lock state

// Timing values for smooth animations
float deltaTime = 0.0f;  // time between frames
float lastFrame = 0.0f;  // timestamp of last frame

// -------------------------
// Input callback declarations
// (implemented at bottom of file)
// -------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);

// -------------------------
// Scene management
// -------------------------
int currentSceneIndex = 0;         // which scene we are currently displaying
std::vector<Scene*> scenes;        // list of scenes

// Struct for sharing state between callbacks
// Instead of using many globals, we put state here
struct AppState {
    std::vector<Scene*>* scenes;
    int* currentSceneIndex;
    bool* mouseCaptured;
    bool* firstMouse;
    Camera* camera;
};

int main()
{
    // -------------------------
    // 1. Initialize GLFW
    // -------------------------
    if (!glfwInit()) {
        std::cerr << "Failed to init GLFW\n";
        return -1;
    }

    // We use OpenGL 3.3 Core profile
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // required on macOS
#endif

    // -------------------------
    // 2. Create a window
    // -------------------------
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Scene", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // -------------------------
    // 3. Register callbacks
    // -------------------------
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetKeyCallback(window, key_callback);

    // Lock the mouse to the window initially
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // -------------------------
    // 4. Initialize GLAD
    // -------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD\n";
        return -1;
    }

    glEnable(GL_DEPTH_TEST); // enable 3D depth testing

    // -------------------------
    // 5. Create and init scenes
    // -------------------------
    DirectionalLightScene* scene1 = new DirectionalLightScene();
    DiffuseAndSpecularMapScene* scene2 = new DiffuseAndSpecularMapScene();
    scenes.push_back(scene1);
    scenes.push_back(scene2);

    // NOTE: We init all scenes up front so switching is instant
    scenes[0]->init();
    scenes[1]->init();

    // Store state for use in callbacks
    AppState appState{ &scenes, &currentSceneIndex, &mouseCaptured, &firstMouse, &camera };
    glfwSetWindowUserPointer(window, &appState);

    // -------------------------
    // 6. Main render loop
    // -------------------------
    while (!glfwWindowShouldClose(window))
    {
        // Timing logic
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Clear screen before rendering
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Poll events + process continuous input
        glfwPollEvents();
        processInput(window);

        // Update and render the active scene
        if (!scenes.empty()) {
            scenes[currentSceneIndex]->update(deltaTime);
            scenes[currentSceneIndex]->render();
        }

        glfwSwapBuffers(window);
    }

    // -------------------------
    // 7. Cleanup memory
    // -------------------------
    for (Scene* s : scenes) {
        delete s;
    }

    glfwTerminate();
    return 0;
}

// -------------------------
// Input handling functions
// -------------------------

// Continuous input (camera movement with WASD)
void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);
}

// Discrete input (actions triggered only once per key press/release)
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    AppState* s = static_cast<AppState*>(glfwGetWindowUserPointer(window));
    if (!s) return;

    // Switch to next scene with → key
    if (key == GLFW_KEY_RIGHT && action == GLFW_RELEASE) {
        if (!s->scenes->empty()) {
            *s->currentSceneIndex = (*s->currentSceneIndex + 1) % s->scenes->size();
            std::cout << "Switched to scene " << *s->currentSceneIndex << '\n';
        }
    }

    // Switch to previous scene with ← key
    if (key == GLFW_KEY_LEFT && action == GLFW_RELEASE) {
        if (!s->scenes->empty()) {
            int n = (int)s->scenes->size();
            *s->currentSceneIndex = (*s->currentSceneIndex + n - 1) % n;
            std::cout << "Switched to scene " << *s->currentSceneIndex << '\n';
        }
    }

    // Toggle mouse capture with ESC
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        if (*s->mouseCaptured) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            *s->mouseCaptured = false;
            std::cout << "Mouse released\n";
        }
        else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            *s->mouseCaptured = true;
            *s->firstMouse = true; // prevent sudden camera jump
            std::cout << "Mouse captured\n";
        }
    }
}

// Resize viewport when window is resized
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    SCR_WIDTH = width;
    SCR_HEIGHT = height;
    glViewport(0, 0, width, height);
}

// Lock mouse back in with left click
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS && !mouseCaptured)
    {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        mouseCaptured = true;
        firstMouse = true;
    }
}

// Camera look-around with mouse
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    if (!mouseCaptured) return;

    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // invert y since OpenGL's origin is bottom-left

    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

// Zoom with scroll wheel
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll((float)yoffset);
}