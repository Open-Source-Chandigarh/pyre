#include <thirdparty/glad/glad.h>
#include <thirdparty/stb_image.h>
#include <iostream>
#include <thirdparty/glm/gtc/matrix_transform.hpp>
#include <thirdparty/glm/gtc/type_ptr.hpp>
#include "scenes/Space.h"
#include "core/ResourceManager.h"
#include "helpers/Utils.h"
#include "core/rendering/geometry/GeometryFactory.h"
#include "core/rendering/GlobalUBO.h"

Space::Space(Window& win)
    : planetShader(nullptr), asteroidShader(nullptr),
    rotationAngle(0.0f), rotationSpeed(50.0f),
    win(win), planet(nullptr), asteroid(nullptr)
{
  
}

void Space::init()
{

    CreateGlobalUBO(); 

    planet = std::make_shared<Model>("resources/models/moon/Moon.fbx");
    asteroid = std::make_shared<Model>("resources/models/rock/rock.obj");

    planetShader = ResourceManager::LoadShader("factory",
        "shaders/modularVertexShader.vs",
        "shaders/modularFragmentShader.fs");

    asteroidShader = ResourceManager::LoadShader("factory",
        "shaders/modularVertexShader.vs",
        "shaders/modularFragmentShader.fs");

    ResourceManager::LoadShader("skybox",
        "shaders/skyBox.vs", "shaders/skyBox.fs");

    std::vector<std::string> faces = {
        "resources/textures/space/right.png",
        "resources/textures/space/left.png",
        "resources/textures/space/top.png",
        "resources/textures/space/bottom.png",
        "resources/textures/space/front.png",
        "resources/textures/space/back.png"
    };

    entities.clear(); // Clear old entities to prevent duplicates on re-init
    std::shared_ptr<Texture> skyBox =
        ResourceManager::LoadCubeMap(faces);
    skyMesh = GeometryFactory::CreateSkyboxCube(); // position-only mesh (36 verts)

    std::shared_ptr<Entity> skyEntity = Entity::Create();
    skyEntity->type = Entity::Type::SkyBox;
    skyEntity->meshRenderer.mesh = &skyMesh;
    skyEntity->meshRenderer.shader = ResourceManager::GetShader("skybox");

    // store cubemap texture in material
    std::shared_ptr<Material> skyMat = std::make_shared<Material>();
    skyMat->textures["skybox"] = skyBox;
    skyEntity->meshRenderer.material = skyMat;
    entities.push_back(std::move(skyEntity));

    unsigned int amount = 100000;
    
    asteroidTransforms.clear(); 
    asteroidTransforms.reserve(amount);
    
    srand(static_cast<unsigned int>(glfwGetTime())); // Cast to avoid warnings
    float radius = 80.0f;
    float offset = 27.0f;

    for (unsigned int i = 0; i < amount; i++)
    {
        glm::mat4 model = glm::mat4(1.0f);
        // 1. Translation: Displace along circle with some random offset
        float angle = (float)i / (float)amount * 360.0f;
        float displacement = (rand() % (int)(2 * offset * 100)) / 100.0f - offset;
        float x = sin(angle) * radius + displacement;
        
        displacement = (rand() % (int)(2 * offset * 100)) / 100.0f - offset;
        float y = displacement * 2.3f; // Keep height variation smaller (disk shape)
        
        displacement = (rand() % (int)(2 * offset * 100)) / 100.0f - offset;
        float z = cos(angle) * radius + displacement;
        
        model = glm::translate(model, glm::vec3(x, y, z));

        // 2. Scale: Randomize size between 0.05 and 0.25
        float scale = static_cast<float>((rand() % 20) / 100.0 + 0.05);
        model = glm::scale(model, glm::vec3(scale));

        // 3. Rotation: Random rotation around a semi-random axis
        float rotAngle = static_cast<float>((rand() % 360));
        model = glm::rotate(model, glm::radians(rotAngle), glm::vec3(0.4f, 0.6f, 0.8f));

        asteroidTransforms.push_back(model);
    }

    // Upload to GPU
    if (asteroid) asteroid->SetupInstancing(asteroidTransforms);

    // Planet
    {
        std::shared_ptr<Entity> e = Entity::Create();
        e->type = Entity::Type::Model;
        e->modelRenderer.model = planet.get();
        e->modelRenderer.shader = planetShader;
        e->transform.position = glm::vec3(0.0f, -3.0f, 0.0f);
        e->transform.scale = glm::vec3(10.0f); // Massive planet
        entities.push_back(e);
    }

    // Asteroid Belt
    {
        std::shared_ptr<Entity> e = Entity::Create();
        e->type = Entity::Type::Model;
        e->modelRenderer.model = asteroid.get();
        e->modelRenderer.shader = asteroidShader; // Use dedicated shader variable
        e->modelRenderer.instanceCount = amount; 
        entities.push_back(e);
    }

    // -------------------------------------------------------------------------
    // LIGHTING SETUP
    // -------------------------------------------------------------------------
    lightManager.ClearPointLights();

    // Sun (Directional)
    glm::vec3 sunDir = glm::normalize(glm::vec3(-1.0f, -0.3f, -0.5f)); 
    lightManager.SetDirectional(
        sunDir,
        glm::vec3(0.02f, 0.02f, 0.02f),
        glm::vec3(1.2f, 1.1f, 1.0f),
        glm::vec3(0.2f)
    );

    // Fill Light
    PointLight fillLight;
    fillLight.position = glm::vec3(50.0f, 20.0f, 50.0f);
    fillLight.ambient  = glm::vec3(0.0f);
    fillLight.diffuse  = glm::vec3(0.1f, 0.1f, 0.3f);
    fillLight.specular = glm::vec3(0.1f);
    fillLight.constant = 1.0f; 
    fillLight.linear = 0.007f; 
    fillLight.quadratic = 0.0002f;
    lightManager.AddPointLight(fillLight);

    // Rim Light
    PointLight rimLight;
    rimLight.position = glm::vec3(-30.0f, 10.0f, -30.0f);
    rimLight.ambient  = glm::vec3(0.0f);
    rimLight.diffuse  = glm::vec3(0.4f, 0.3f, 0.2f) * 2.0f;
    rimLight.specular = glm::vec3(0.3f);
    rimLight.constant = 1.0f; 
    rimLight.linear = 0.022f; 
    rimLight.quadratic = 0.0019f;
    lightManager.AddPointLight(rimLight);

    // Flashlight
    SpotLight s;
    s.position = win.GetAppState()->camera.Position;
    s.direction = win.GetAppState()->camera.Front;
    s.ambient = glm::vec3(0.0f);
    s.diffuse = glm::vec3(1.0f, 1.0f, 1.0f);
    s.specular = glm::vec3(0.5f);
    s.constant = 1.0f;
    s.quadratic = 0.032f;
    s.linear = 0.09f;
    s.innerCutOff = cos(glm::radians(12.5f));
    s.outerCutOff = cos(glm::radians(17.5f));
    lightManager.AddSpotLight(s);
}

void Space::update() { }

void Space::render()
{
    auto app = win.GetAppState();
    if (!app) return;

    glm::mat4 view = app->camera.GetViewMatrix();
    glm::mat4 proj = glm::perspective(glm::radians(app->camera.Zoom),
        (float)win.Width() / (float)win.Height(), 0.1f, 1000.0f);

    renderer.BeginScene(view, proj, app->camera.Position);
    lightManager.UploadToUBO(view, proj, app->camera.Position);

    // Handles both Planet and Asteroids automatically.
    renderer.RenderScene(entities, app->camera);

    renderer.EndScene();
}