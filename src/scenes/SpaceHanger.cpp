#include "scenes/SpaceHanger.h"
#include "core/ResourceManager.h"
#include "core/rendering/Material.h"
#include "core/rendering/Texture.h"
#include "core/rendering/geometry/GeometryFactory.h"
#include "helpers/Utils.h"
#include <iostream>
#include <thirdparty/glad/glad.h>
#include <thirdparty/glm/gtc/matrix_transform.hpp>
#include <thirdparty/glm/gtc/type_ptr.hpp>
#include <thirdparty/stb_image.h>

SpaceHanger::SpaceHanger()
{
}
SpaceHanger::~SpaceHanger()
{
}

void SpaceHanger::Init(AppState &appState)
{
    obj = std::make_shared<Model>("resources\\models\\space\\spacehanger.gltf");
    if (!obj)
        return;

    // shaders
    shader =
        ResourceManager::LoadShader("factory", "shaders/modularVertexShader.vs", "shaders/modularFragmentShader.fs");

    entities.clear();

    auto skyBox = ResourceManager::LoadIBLCubeMap("resources\\earthlike_planet.hdr");
    auto irradianceMap = ResourceManager::ConvoluteIrradianceMap(skyBox);
    auto prefilterMap = ResourceManager::PreFilterEnvironmentMap(skyBox);
    ResourceManager::LoadShader("skybox", "shaders/common/skyBox.vs", "shaders/common/skyBox.fs");

    skyMesh = GeometryFactory::CreateSkyboxCube();

    {
        auto skyMat = std::make_shared<Material>();
        skyMat->textures["skybox"] = skyBox;
        skyMat->textures["irradiance"] = irradianceMap;
        skyMat->textures["prefilter"] = prefilterMap;
        std::shared_ptr<Entity> e = Entity::Create("Skybox");
        e->AddSkybox(skyMesh.get(), skyMat, ResourceManager::GetShader("skybox"));
        entities.push_back(e);
    }

    std::shared_ptr<Entity> e = Entity::Create("Hanger");
    e->AddModel(obj.get(), shader);
    e->transform.position = glm::vec3(0.0f);
    entities.push_back(std::move(e));
}

void SpaceHanger::OnActivate(AppState &appState)
{
    // Lights
    appState.lightManager->ClearPointLights();
    appState.lightManager->ClearSpotLights();

    appState.lightManager->SetDirectional(glm::vec3(-0.5f, -1.0f, -0.3f), glm::vec3(0.05f), glm::vec3(0.6f),
                                          glm::vec3(0.2f));

    PointLight p;
    p.position = glm::vec3(0.7f, 0.2f, 2.0f);
    p.diffuse = glm::vec3(0.2f, 1.5f, 1.5f) * 0.8f;
    p.radius = 100.0f;
    appState.lightManager->AddPointLight(p);

    PointLight p2;
    p2.position = glm::vec3(2.3f, -3.3f, -4.0f);
    p2.diffuse = glm::vec3(2.0f, 1.5f, 2.0f) * 0.9f;
    p2.radius = 100.0f;
    appState.lightManager->AddPointLight(p2);

    PointLight p3;
    p3.position = glm::vec3(-4.0f, 2.0f, -12.0f);
    p3.diffuse = glm::vec3(1.0f, 7.0f, 0.6f) * 0.8f;
    p3.radius = 100.0f;
    appState.lightManager->AddPointLight(p3);
}

void SpaceHanger::Update(AppState &appState)
{
}