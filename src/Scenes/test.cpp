#include "scenes/Test.h"
#include "application/AppState.h"
#include "core/ResourceManager.h"
#include "core/rendering/geometry/GeometryFactory.h" 
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

    auto floorDiffuseMap = ResourceManager::LoadTexture("resources/textures/bricks.jpg", TextureType::TEX_DIFFUSE);
    auto floorNormalMap = ResourceManager::LoadTexture("resources/textures/bricks_normal.jpg", TextureType::TEX_NORMAL);
    auto floorDisplacementMap = ResourceManager::LoadTexture("resources/textures/bricks_disp.jpg", TextureType::TEX_DISPLACEMENT);
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
        e -> AddSkybox(skyMesh.get(), skyMat, ResourceManager::GetShader("skybox"));
        entities.push_back(e);
    }

    {
        std::shared_ptr<Material> mat = std::make_shared<Material>();
        mat -> cullMode = CullMode::None;
        mat -> isTransparent = true;
        mat -> textures["material_diffuse"] = grassDiffuseMap;
        mat -> floats["material_shininess"] = 16.0f;

        std::shared_ptr<Entity> e = Entity::Create();
        e -> AddMesh(plane, mat, shader);
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
        e -> AddMesh(plane, mat, shader);
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
                e -> AddMesh(cube, cubeMat, shader);
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
        mat -> textures["material_normal"] = floorNormalMap;
        mat -> textures["material_displacement"] = floorDisplacementMap;
        mat -> floats["material_shininess"] = 164.0f;

        std::shared_ptr<Entity> e = Entity::Create();
        e -> AddMesh(plane, mat, shader);
        e -> transform.scale = glm::vec3(10);
        entities.push_back(e);
    }
}

void Test::OnActivate(AppState &appState)
{
    // Reset Camera
    appState.camera.Position = glm::vec3(0.0f, 8.0f, 15.0f);

    appState.lightManager->ClearPointLights();
    appState.lightManager->ClearSpotLights();
  
    appState.lightManager->SetDirectional(
        glm::vec3(0.0f),
        glm::vec3(0.0f),
        glm::vec3(0.0f),
        glm::vec3(0.0f)
    );

    PointLight k;
    k.position = glm::vec3(1.5f, 2, 1.5f);
    k.ambient = glm::vec3(0.05f); // Ambient should be tiny (e.g. 0.01 to 0.1)
    k.diffuse = glm::vec3(10.0f); // 5.0 to 10.0 is plenty bright for HDR
    k.specular = glm::vec3(10.0f);
    k.constant = 1; k.linear = 0.09f; k.quadratic = 0.032f;
    appState.lightManager -> AddPointLight(k);

    PointLight s;
    s.position = glm::vec3(-3.0f, 1.5f, 0.0f);
    s.ambient = glm::vec3(0.03f);
    s.diffuse = glm::vec3(0.8f, 0.9f, 0.2f);
    s.specular = glm::vec3(0.2f);
    s.constant = 1; s.linear = 0.09f; s.quadratic = 0.032f;
    appState.lightManager -> AddPointLight(s);

    // PointLight f;
    // f.position = glm::vec3(-1, 2, 1);
    // f.ambient = glm::vec3(0.04f);
    // f.diffuse = glm::vec3(0.7, 0.4, 0.1);
    // f.specular = glm::vec3(0.4f);
    // f.constant = 1; f.linear = 0.14f; f.quadratic = 0.07f;
    // appState.lightManager -> AddPointLight(f);

    // PointLight r;
    // r.position = glm::vec3(-1, 2, -2);
    // r.ambient = glm::vec3(0.01f);
    // r.diffuse = glm::vec3(0.2, 0.5, 0.4);
    // r.specular = glm::vec3(0.2f);
    // r.constant = 1; r.linear = 0.09f; r.quadratic = 0.032f;
    // appState.lightManager->AddPointLight(r);

    appState.lightManager->ShowDebugLights(true);
}

void Test::Update(AppState &appState) 
{
    float time = (float)glfwGetTime();

    // if (!appState.lightManager->points.empty())
    // {
    //     for(unsigned int i = 0; i < appState.lightManager->points.size(); i++)
    //     {
    //         // Orbit radius 10 around the crate stack
    //         float radius = 2.0f;
            
    //         // Move in a circle
    //         float x = sin(time * 0.5f * (i + 1)) * radius * (i + 1);
    //         float z = cos(time * 0.5f * (i + 1)) * radius * (i + 1);
            
    //         // Bob up and down slightly (height 4 to 8) to change shadow length
    //         float y = 1.5f + (i + 1);

    //         appState.lightManager->points[i].position = glm::vec3(x, y, z);
    //     }
    // }
}