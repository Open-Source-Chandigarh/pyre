#include "application/AppState.h"
#include "core/LightManager.h"
#include "core/rendering/Renderer.h"
#include "scenes/Scene.h"

AppState::AppState()
{
    renderer = std::make_unique<Renderer>();
    lightManager = std::make_unique<LightManager>();
}

AppState::~AppState() = default;