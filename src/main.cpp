#include "application/Application.h"
#include "core/ResourceManager.h"
#include "scenes/Space.h"
#include "scenes/ToonScene.h"
#include "scenes/backpack.h"
#include "scenes/factoryScene.h"
#include "scenes/test.h"
#include <iostream>

int main()
{
    try
    {
        Application app("Pyre Engine", 800, 600);

        app.AddScene(new FactoryScene());
        app.AddScene(new Backpack());
        app.AddScene(new ToonScene());
        app.AddScene(new Space());
        app.AddScene(new Test());

        app.Run();
    }
    catch (const std::exception &e)
    {
        std::cerr << "ERROR: " << e.what() << std::endl;
        ResourceManager::Clear();
        return -1;
    }

    ResourceManager::Clear();
    return 0;
}