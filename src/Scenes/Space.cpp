#include "scenes/Space.h"
#include "application/AppState.h"
#include "core/ResourceManager.h"
#include "helpers/Utils.h"
#include "core/rendering/geometry/GeometryFactory.h"
#include "core/rendering/Texture.h"
#include "core/rendering/Material.h"
#include "core/rendering/Renderer.h"
#include "core/LightManager.h"

Space::Space()
    : planetShader(nullptr), asteroidShader(nullptr),
    rotationAngle(0.0f), rotationSpeed(50.0f),
    planet(nullptr), asteroid(nullptr)
{}

Space::~Space() {}

void Space::Init(AppState &appState)
{
    planet = std::make_shared<Model>("resources/models/moon/Moon.fbx");
    asteroid = std::make_shared<Model>("resources/models/rock/rock.obj");

    planetShader = ResourceManager::LoadShader("factory", "shaders/modularVertexShader.vs", "shaders/modularFragmentShader.fs");
    asteroidShader = ResourceManager::LoadShader("factory", "shaders/modularVertexShader.vs", "shaders/modularFragmentShader.fs");
    ResourceManager::LoadShader("skybox", "shaders/common/skyBox.vs", "shaders/common/skyBox.fs");

    std::vector<std::string> faces = {
        "resources/textures/space/right.png", "resources/textures/space/left.png",
        "resources/textures/space/top.png", "resources/textures/space/bottom.png",
        "resources/textures/space/front.png", "resources/textures/space/back.png"
    };

    entities.clear();
    
    std::shared_ptr<Texture> skyBox = ResourceManager::LoadCubeMap(faces);
    skyMesh = GeometryFactory::CreateSkyboxCube();

    std::shared_ptr<Material> skyMat = std::make_shared<Material>();
    skyMat -> textures["skybox"] = skyBox;
    
    std::shared_ptr<Entity> skyEntity = Entity::Create();
    skyEntity -> AddSkybox(&skyMesh, skyMat, ResourceManager::GetShader("skybox"));
    entities.push_back(std::move(skyEntity));

    unsigned int amount = 100000;
    asteroidTransforms.clear(); 
    asteroidTransforms.reserve(amount);
    
    srand(static_cast<unsigned int>(glfwGetTime()));
    float radius = 110.0f;
    float offset = 50.0f;

    for (unsigned int i = 0; i < amount; i++)
    {
        glm::mat4 model = glm::mat4(1.0f);
        float angle = (float)i / (float)amount * 360.0f;
        float displacement = (rand() % (int)(2 * offset * 100)) / 100.0f - offset;
        float x = sin(angle) * radius + displacement;
        
        displacement = (rand() % (int)(2 * offset * 100)) / 100.0f - offset;
        float y = displacement * 4.0f;
        
        displacement = (rand() % (int)(2 * offset * 100)) / 100.0f - offset;
        float z = cos(angle) * radius + displacement;
        model = glm::translate(model, glm::vec3(x, y, z));

        float scale = static_cast<float>((rand() % 20) / 100.0 + 0.05);
        model = glm::scale(model, glm::vec3(scale));

        float rotAngle = static_cast<float>((rand() % 360));
        model = glm::rotate(model, glm::radians(rotAngle), glm::vec3(0.4f, 0.6f, 0.8f));

        asteroidTransforms.push_back(model);
    }

    if (asteroid) asteroid -> SetupInstancing(asteroidTransforms);

    {
        std::shared_ptr<Entity> e = Entity::Create();
        e -> AddModel(planet.get(), planetShader);
        e -> transform.position = glm::vec3(0.0f, -3.0f, 0.0f);
        e -> transform.scale = glm::vec3(10.0f); 
        entities.push_back(e);
    }

    {
        std::shared_ptr<Entity> e = Entity::Create();
        e -> AddModel(asteroid.get(), planetShader, amount);
        entities.push_back(e);
    }

    appState.lightManager -> ClearPointLights();
}

void Space::OnActivate(AppState &appState)
{
    appState.camera.Position = glm::vec3(0.0f, 10.0f, 30.0f);
    appState.camera.Near = 0.1f;
    appState.camera.Far = 2000.0f;
    // Lights 
    appState.lightManager -> ClearPointLights();
    appState.lightManager -> ClearSpotLights();

    glm::vec3 sunDir = glm::normalize(glm::vec3(-1.0f, -0.3f, -0.5f)); 
    appState.lightManager -> SetDirectional(sunDir, glm::vec3(0.02f), 
                                            glm::vec3(1.2f, 1.1f, 1.0f), 
                                            glm::vec3(0.2f));

    PointLight fillLight;
    fillLight.position = glm::vec3(50.0f, 20.0f, 50.0f);
    fillLight.ambient  = glm::vec3(0.0f);
    fillLight.diffuse  = glm::vec3(0.1f, 0.1f, 0.3f);
    fillLight.specular = glm::vec3(0.1f);
    fillLight.constant = 1.0f; fillLight.linear = 0.007f; fillLight.quadratic = 0.0002f;
    appState.lightManager -> AddPointLight(fillLight);

    PointLight rimLight;
    rimLight.position = glm::vec3(-30.0f, 10.0f, -30.0f);
    rimLight.ambient  = glm::vec3(0.0f);
    rimLight.diffuse  = glm::vec3(0.4f, 0.3f, 0.2f) * 2.0f;
    rimLight.specular = glm::vec3(0.3f);
    rimLight.constant = 1.0f; rimLight.linear = 0.022f; rimLight.quadratic = 0.0019f;
    appState.lightManager -> AddPointLight(rimLight);
}

void Space::Update(AppState &appState) {}