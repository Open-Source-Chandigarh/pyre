#pragma once
#include <memory>
#include <vector>
#include <string>
#include "helpers/InputMapper.h"

class Window;
struct AppState;
class Scene;

class Application
{
public:
    Application(const std::string& title, int width, int height);
    ~Application();

    void AddScene(Scene *scene);
    // The entry point to start the game loop
    void Run();

private:
    // core Subsystems
    std::unique_ptr<Window> window;
    std::unique_ptr<AppState> appState;
    InputMapper inputMapper;

    // internal Helpers
    void Init();       // setup scenes, render defaults
    void ConfigureInput(); // bind keys to AppState logic
    void Update(float dt);
    void Render();
};
