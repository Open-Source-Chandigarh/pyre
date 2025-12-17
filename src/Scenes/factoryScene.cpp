#include "scenes/factoryScene.h"
#include "application/AppState.h"
#include "core/ResourceManager.h"
#include "helpers/Utils.h"
#include "core/rendering/geometry/GeometryFactory.h"
#include "core/rendering/GlobalUBO.h"
#include "core/rendering/Material.h"
#include "core/rendering/Texture.h"
#include "core/rendering/Renderer.h"
#include "core/LightManager.h"

FactoryScene::FactoryScene()
    : shader(nullptr),
    rotationAngle(0.0f), rotationSpeed(50.0f)
{
    cubePositions[0] = glm::vec3(0.0f, 0.0f, 0.0f);
    cubePositions[1] = glm::vec3(2.0f, 5.0f, -15.0f);
    cubePositions[2] = glm::vec3(-1.5f, -2.2f, -2.5f);
    cubePositions[3] = glm::vec3(-3.8f, -2.0f, -12.3f);
    cubePositions[4] = glm::vec3(2.4f, -0.4f, -3.5f);
    cubePositions[5] = glm::vec3(-1.7f, 3.0f, -7.5f);
    cubePositions[6] = glm::vec3(1.3f, -2.0f, -2.5f);
    cubePositions[7] = glm::vec3(1.5f, 2.0f, -2.5f);
    cubePositions[8] = glm::vec3(1.5f, 0.2f, -1.5f);
    cubePositions[9] = glm::vec3(-1.3f, 1.0f, -1.5f);
}

FactoryScene::~FactoryScene() {}

void FactoryScene::Init(AppState &appState)
{
    entities.clear();
    
    // shaders
    shader = ResourceManager::LoadShader("factory",
        "shaders/modularVertexShader.vs",
        "shaders/modularFragmentShader.fs");

    diffuseMap = ResourceManager::LoadTexture(
        "resources/textures/metalDiff.png", TextureType::TEX_DIFFUSE);
    specularMap = ResourceManager::LoadTexture(
        "resources/textures/metalSpec.png", TextureType::TEX_SPECULAR);

    ResourceManager::LoadShader("skybox", "shaders/common/skyBox.vs", "shaders/common/skyBox.fs");

    std::vector<std::string> faces = {
        "resources/textures/stylizedSky/front.png",
        "resources/textures/stylizedSky/back.png",
        "resources/textures/stylizedSky/top.png",
        "resources/textures/stylizedSky/bottom.png",
        "resources/textures/stylizedSky/right.png",
        "resources/textures/stylizedSky/left.png",
    };
    std::shared_ptr<Texture> skyBox = ResourceManager::LoadCubeMap(faces);
    skyMesh = GeometryFactory::CreateSkyboxCube();

    std::shared_ptr<Material> skyMat = std::make_shared<Material>();
    skyMat -> textures["skybox"] = skyBox;

    std::shared_ptr<Entity> skyEntity = Entity::Create();
    skyEntity -> AddSkybox(&skyMesh, skyMat, ResourceManager::GetShader("skybox"));
    entities.push_back(std::move(skyEntity));

    // Create random shapes 
    for (int i = 0; i < 10; ++i)
    {
        int randomInt = Utils::RandomInt(0, 4);
        if (randomInt == 0) mesh[i] = GeometryFactory::CreateSphere();
        else if (randomInt == 1) mesh[i] = GeometryFactory::CreateCube();
        else if (randomInt == 2) mesh[i] = GeometryFactory::CreateTorus();
        else if (randomInt == 3) mesh[i] = GeometryFactory::CreateCube();
        else mesh[i] = GeometryFactory::CreateCone();
    }

    // Create Entities 
    for (int i = 0; i < 10; ++i)
    {
        std::shared_ptr<Material> mat = std::make_shared<Material>();
        mat -> textures["material_diffuse"] = diffuseMap;
        mat -> textures["material_specular"] = specularMap;
        mat -> textures["material_skybox"] = skyBox;
        mat -> floats["material_shininess"] = 256.0f; 
        mat -> floats["material_reflectivity"] = 0.6f; 

        std::shared_ptr<Entity> e = Entity::Create();
        e -> AddMesh(&mesh[i], mat, shader);
        e -> transform.position = cubePositions[i];
        e -> transform.scale = glm::vec3(0.7f);
        entities.push_back(std::move(e));
    }

  
}

void FactoryScene::OnActivate(AppState &appState)
{
    // Lights 
    appState.lightManager -> ClearPointLights();
    appState.lightManager -> ClearSpotLights();

    appState.lightManager -> SetDirectional(glm::vec3(-0.5f, -1.0f, -0.5f),
        glm::vec3(0.05f), glm::vec3(0.1f, 0.15f, 0.3f), glm::vec3(0.2f));

    PointLight keyLight;
    keyLight.position = glm::vec3(0.0f, 2.0f, 2.0f);
    keyLight.ambient = glm::vec3(0.0f); 
    keyLight.diffuse = glm::vec3(1.0f, 0.6f, 0.3f) * 1.5f; 
    keyLight.specular = glm::vec3(1.0f, 0.8f, 0.6f); 
    keyLight.constant = 1.0f; keyLight.linear = 0.09f; keyLight.quadratic = 0.032f;
    appState.lightManager -> AddPointLight(keyLight);

    PointLight rimLight;
    rimLight.position = glm::vec3(-3.0f, 1.0f, -5.0f);
    rimLight.ambient = glm::vec3(0.0f);
    rimLight.diffuse = glm::vec3(0.0f, 0.5f, 1.0f) * 1.0f; 
    rimLight.specular = glm::vec3(0.0f, 1.0f, 1.0f);
    rimLight.constant = 1.0f; rimLight.linear = 0.09f; rimLight.quadratic = 0.032f;
    appState.lightManager -> AddPointLight(rimLight);
}

void FactoryScene::Update(AppState &appState) 
{
    float time = (float)glfwGetTime();
    
    int shapeIndex = 0;
    for (size_t i = 0; i < entities.size(); ++i) 
    {
        if(entities[i] -> skyboxComp) continue; // Skip skybox

        if (shapeIndex >= 10) break; 

        float yOffset = sin(time * 0.5f + shapeIndex) * 0.5f; // use shapeIndex for wave offset
    
        entities[i] -> transform.position = cubePositions[shapeIndex]; 
        entities[i] -> transform.position.y += yOffset;
        entities[i] -> transform.rotation.x = time * 5.0f * (shapeIndex % 2 == 0 ? 1 : -1);
        entities[i] -> transform.rotation.y = time * 3.0f;
        
        shapeIndex++;
    }
}

void FactoryScene::Render(AppState &appState)
{
    Renderer *renderer = appState.renderer.get();

    if (!appState.lightManager -> spots.empty()) 
    {
        appState.lightManager -> spots[0].position = appState.camera.Position;
        appState.lightManager -> spots[0].direction = appState.camera.Front;
    }

     renderer -> BeginScene(appState.camera, 
                            *appState.globalUBO, 
                            *appState.lightManager, 
                            appState.GetAspectRatio());

    renderer -> RenderScene(entities, appState.camera, *appState.lightManager, 
                            *sceneFBO, *postPipeline, appState.wireframeEnabled);

    renderer -> EndScene();
}