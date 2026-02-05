#include "application/Application.h"
#include "application/AppState.h"
#include "core/Window.h"
#include "core/InputManager.h"
#include "core/ResourceManager.h"
#include "scenes/Scene.h"
#include "scenes/FactoryScene.h"
#include "scenes/Backpack.h"
#include "scenes/Space.h"
#include "scenes/ToonScene.h"
#include "scenes/Test.h"

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

Application::Application(const std::string& title, int width, int height)
{
    window = std::make_unique<Window>(width, height, title);
    appState = std::make_unique<AppState>();
}

Application::~Application()
{
    // Cleanup ImGui
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    ResourceManager::Clear(); 
    
    // appState and window will be destroyed automatically by unique_ptr
    // appState first (because it was declared 2nd), then window
    // this is needed because AppState components might need GL context to delete buffers
}

void Application::Init()
{
    for (auto& scene : appState->scenes)
        scene->BindWindow(window.get());

    for (auto& scene : appState->scenes) 
        scene->Init(*appState);

    if (!appState->scenes.empty()) {
        appState->scenes[appState->currentSceneIndex]->OnActivate(*appState);
        appState->camera.SetDefault();
    }

    ConfigureInput();
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Setup ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls

    // Setup ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window->GetNative(), true);
    ImGui_ImplOpenGL3_Init("#version 420");
}

void Application::ConfigureInput()
{
    InputManager *input = window->GetInputManager();
    AppState *app = appState.get();
    Window *winPtr = window.get();

    // camera Movement
    input->BindKeyContinuous(GLFW_KEY_W, [app](float dt) { 
        app->camera.ProcessKeyboard(FORWARD, dt); 
    });
    input->BindKeyContinuous(GLFW_KEY_S, [app](float dt) { 
        app->camera.ProcessKeyboard(BACKWARD, dt); 
    });
    input->BindKeyContinuous(GLFW_KEY_A, [app](float dt) { 
        app->camera.ProcessKeyboard(LEFT, dt); 
    });
    input->BindKeyContinuous(GLFW_KEY_D, [app](float dt) { 
        app->camera.ProcessKeyboard(RIGHT, dt); 
    });

    // mouse
    input->BindMouseMove([app](double x, double y) {
        app->camera.ProcessMouseMovement((float)x, (float)y);
    });
    input->BindScroll([app](double y) {
        app->camera.ProcessMouseScroll((float)y);
    });
    input->BindKeyEvent(GLFW_KEY_ESCAPE, GLFW_PRESS, [input]() {
        input->ToggleMouseCapture();
    });

    // scene switching
    auto SwitchScene = [app, winPtr](int offset) {
        int n = (int)app->scenes.size();
        if (n == 0) return;
        app->currentSceneIndex = (app->currentSceneIndex + offset + n) % n;
        glfwSetWindowTitle(winPtr->GetNative(), app->scenes[app->currentSceneIndex]->Name().c_str());
        app->camera.Reset(); // Clear any extreme movement/rotation
        app->scenes[app->currentSceneIndex]->OnActivate(*app);
        app->camera.SetDefault(); // Lock in the scene's starting camera state
    };

    input->BindKeyEvent(GLFW_KEY_RIGHT, GLFW_RELEASE, 
        [SwitchScene](){ SwitchScene(1); });
    input->BindKeyEvent(GLFW_KEY_LEFT, GLFW_RELEASE, 
        [SwitchScene](){ SwitchScene(-1); });

    // camera reset
    input->BindKeyEvent(GLFW_KEY_R, GLFW_RELEASE, [app]() {
        app->camera.Reset();
    });

    // debug options
    input->BindKeyEvent(GLFW_KEY_F, GLFW_RELEASE, [app]() {
        app->wireframeEnabled = !app->wireframeEnabled;
        glPolygonMode(GL_FRONT_AND_BACK, 
            app->wireframeEnabled ? GL_LINE : GL_FILL);
    });
}

void Application::Update(float dt)
{
    // Update Input
    window->GetInputManager()->Update(dt);

    // Update Current Scene
    if (!appState->scenes.empty()) {
        appState->scenes[appState->currentSceneIndex]->Update(*appState);
    }
}

void Application::Render()
{
    if (appState->scenes.empty()) return;

    Scene *activeScene = appState->scenes[appState->currentSceneIndex].get();
    glm::vec3 col = activeScene->clearColor;

    glClearColor(col.r, col.g, col.b, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    activeScene->Render(*appState);
}

void Application::Run()
{
    Init();
    // Track resizing logic
    int lastW = window->Width();
    int lastH = window->Height();

    while (!window->ShouldClose())
    {
        // time step
        float currentFrame = static_cast<float>(glfwGetTime());
        appState->deltaTime = currentFrame - appState->lastFrame;
        appState->lastFrame = currentFrame;

        // handle Resize
        if (window->Width() != lastW || window->Height() != lastH) {
            lastW = window->Width();
            lastH = window->Height();
            glViewport(0, 0, lastW, lastH);
            appState->width = lastW;
            appState->height = lastH;
            for (auto& s : appState->scenes) s->OnResize(lastW, lastH);
        }

        Update(appState->deltaTime);
        Render();

        // Render UI
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Pyre Editor");
        ImGui::Text("Application Average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        ImGui::End();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        window->SwapBuffers();
        window->PollEvents();
    }
}

void Application::AddScene(Scene *scene)
{
    // take ownership convert raw pointer to unique_ptr
    appState->scenes.push_back(std::unique_ptr<Scene>(scene));
}