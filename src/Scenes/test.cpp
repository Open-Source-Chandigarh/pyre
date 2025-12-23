#include "scenes/Test.h"
#include "application/AppState.h"
#include "core/ResourceManager.h"
#include "core/rendering/geometry/GeometryFactory.h"
#include "core/rendering/GlobalUBO.h"   
#include "helpers/Shader.h"
#include "core/rendering/Material.h"
#include "core/rendering/Renderer.h"
#include "core/LightManager.h"

Test::Test() : shader(nullptr) {}
Test::~Test() {}

void Test::Init(AppState &appState)
{
    entities.clear();

    shader = ResourceManager::LoadShader("test", "shaders/modularVertexShader.vs", "shaders/modularFragmentShader.fs");
    ResourceManager::LoadShader("skybox", "shaders/common/skyBox.vs", "shaders/common/skyBox.fs");

    auto floorDiffuseMap = ResourceManager::LoadTexture("resources/textures/woodDiff.png", TextureType::TEX_DIFFUSE);
    auto floorSpecularMap = ResourceManager::LoadTexture("resources/textures/woodSpec.png", TextureType::TEX_SPECULAR);
    auto cubeDiffuseMap = ResourceManager::LoadTexture("resources/textures/crateDiff.jpg", TextureType::TEX_DIFFUSE);
    auto cubeSpecularMap = ResourceManager::LoadTexture("resources/textures/crateSpec.jpg", TextureType::TEX_SPECULAR);
    auto grassDiffuseMap = ResourceManager::LoadTexture("resources/textures/grass.png", TextureType::TEX_DIFFUSE);
    auto windowDiffuseMap = ResourceManager::LoadTexture("resources/textures/transparent_window.png", TextureType::TEX_DIFFUSE);
    auto windowSpecMap = ResourceManager::LoadTexture("resources/textures/metalSpec.png", TextureType::TEX_SPECULAR);

    std::vector<std::string> faces = {
        "resources/textures/skybox/right.jpg", "resources/textures/skybox/left.jpg",
        "resources/textures/skybox/top.jpg", "resources/textures/skybox/bottom.jpg",
        "resources/textures/skybox/front.jpg", "resources/textures/skybox/back.jpg"
    };
    auto skyBox = ResourceManager::LoadCubeMap(faces);

    cube = GeometryFactory::CreateCube();
    plane = GeometryFactory::CreatePlane();
    skyMesh = GeometryFactory::CreateSkyboxCube();
    

    {
        auto skyMat = std::make_shared<Material>();
        skyMat -> textures["skybox"] = skyBox; 
        std::shared_ptr<Entity> e = Entity::Create();
        e -> AddSkybox(&skyMesh, skyMat, ResourceManager::GetShader("skybox"));
        entities.push_back(e);
    }

    {
        std::shared_ptr<Material> mat = std::make_shared<Material>();
        mat -> cullMode = CullMode::None;
        mat -> isTransparent = true;
        mat -> textures["material_diffuse"] = grassDiffuseMap;
        mat -> floats["material_shininess"] = 16.0f;

        std::shared_ptr<Entity> e = Entity::Create();
        e -> AddMesh(&plane, mat, shader);
        e -> transform.position = glm::vec3(1.5f, 0.5f, 4.0f);
        e -> transform.rotation = glm::vec3(270, 0, 0);
        e -> transform.scale = glm::vec3(1);
        entities.push_back(e);
    }

    {
        std::shared_ptr<Material> mat = std::make_shared<Material>();
        mat -> isTransparent = true;
        mat -> cullMode = CullMode::None;
        mat -> textures["material_diffuse"] = windowDiffuseMap;
        mat -> textures["material_specular"] = windowSpecMap;
        mat -> floats["material_shininess"] = 100.0f;

        std::shared_ptr<Entity> e = Entity::Create();
        e -> AddMesh(&plane, mat, shader);
        e -> transform.position = glm::vec3(0.0f, 0.5f, 4.0f);
        e -> transform.rotation = glm::vec3(270, 0, 0);
        e -> transform.scale = glm::vec3(1);
        entities.push_back(e);
    }

    std::shared_ptr<Material> cubeMat = std::make_shared<Material>();
    cubeMat -> textures["material_diffuse"] = cubeDiffuseMap;
    cubeMat -> textures["material_specular"] = cubeSpecularMap;
    cubeMat -> floats["material_shininess"] = 32.0f;
    cubeMat -> vec3s["material_diffuseColor"] = glm::vec3(1.0f);
    cubeMat -> vec3s["material_specularColor"] = glm::vec3(1.0f);
    // cubeMat -> showNormals = true;
    // cubeMat -> outlineEnabled = true;

    float spacing = 1.02f;
    float height = 1.1f;
    int base = 2;

    for (int layer = 0; layer < base; layer++)
    {
        float y = 0.55f + layer * height;
        float start = -(base - layer - 1) * (spacing / 2.0f);

        for (int i = 0; i < base - layer; i++)
        {
            for (int j = 0; j < base - layer; j++)
            {
                std::shared_ptr<Entity> e = Entity::Create();
                e -> AddMesh(&cube, cubeMat, shader);
                e -> transform.position = glm::vec3(start + i * spacing, y, start + j * spacing);
                e -> transform.scale = glm::vec3(1.1f);
                entities.push_back(e);
            }
        }
    }

    {
        std::shared_ptr<Material> mat = std::make_shared<Material>();
        mat -> cullMode = CullMode::Front;
        mat -> textures["material_diffuse"] = floorDiffuseMap;
        mat -> textures["material_specular"] = floorSpecularMap;
        mat -> floats["material_shininess"] = 164.0f;

        std::shared_ptr<Entity> e = Entity::Create();
        e -> AddMesh(&plane, mat, shader);
        e -> transform.scale = glm::vec3(20);
        entities.push_back(e);
    }


}

void Test::OnActivate(AppState &appState)
{
    // Lights 
    appState.lightManager -> ClearPointLights();
    appState.lightManager -> ClearSpotLights();
    
    appState.lightManager -> SetDirectional(
        glm::vec3(-0.5f, -1.0f, -0.3f), glm::vec3(0.05f), glm::vec3(0.6f), glm::vec3(0.2f)
    );

}

void Test::Update(AppState &appState) 
{
    float time = (float)glfwGetTime() * 0.8f;

    // varying X and Z creates a circle around the scene
    float lightX = sin(time); 
    float lightZ = cos(time) * 1.2f;
    
    // Y is set to -1.0f so it always points somewhat downwards at the floor
    glm::vec3 newDirection = glm::normalize(glm::vec3(lightX, -1.0f, lightZ));

    // We keep the color/intensity values consistent with what you set in OnActivate
    appState.lightManager->SetDirectional(
        newDirection,
        glm::vec3(0.05f),        // Ambient
        glm::vec3(0.4f),        // Diffuse
        glm::vec3(0.1f)         // Specular
    );

    if (!appState.lightManager -> spots.empty())
    {
        appState.lightManager -> spots[0].position = appState.camera.Position;
        appState.lightManager -> spots[0].direction = appState.camera.Front;
    }
}