#include "application/AppState.h"
#include "core/rendering/Renderer.h"
#include "core/LightManager.h"
#include "core/rendering/GlobalUBO.h"
#include "scenes/Scene.h"

AppState::AppState() 
{
    renderer = std::make_unique<Renderer>();
    lightManager = std::make_unique<LightManager>();
    globalUBO = std::make_unique<GlobalUBO>();
}

AppState::~AppState() = default;