#include "scenes/Test.h"
#include "core/ResourceManager.h"
#include "core/rendering/geometry/GeometryFactory.h"
#include "core/rendering/GlobalUBO.h"   
#include "helpers/Shader.h"
#include "core/rendering/Material.h"

Test::Test(Window& win) : Scene(win), shader(nullptr)
{
    renderer = std::make_shared<Renderer>();
    lightManager = std::make_shared<LightManager>();
     // Create scene framebuffer once
    sceneFBO = std::make_shared<Framebuffer>((unsigned int)win.Width(), 
        (unsigned int)win.Height(), true, true);

    // Create post processing pipeline and add effects
    postPipeline = std::make_shared<PostProcessingPipeline>((unsigned int)win.Width(), 
        (unsigned int)win.Height());

    postPipeline->AddGammaCorrection(2.2f);
}

void Test::init()
{
    entities.clear();
    // Make sure global UBO system is created ONCE
    CreateGlobalUBO();

    // Load modular shaders (with include preprocessor)
    shader = ResourceManager::LoadShader(
        "test",
        "shaders/modularVertexShader.vs",
        "shaders/modularFragmentShader.fs"
    );

    // Skybox shader
    ResourceManager::LoadShader(
        "skybox",
        "shaders/skyBox.vs",
        "shaders/skyBox.fs"
    );

    // Load textures
    auto floorDiffuseMap = ResourceManager::LoadTexture("resources/textures/woodDiff.png", TextureType::TEX_DIFFUSE);
    auto floorSpecularMap = ResourceManager::LoadTexture("resources/textures/woodSpec.png", TextureType::TEX_SPECULAR);

    auto cubeDiffuseMap = ResourceManager::LoadTexture("resources/textures/crateDiff.jpg", TextureType::TEX_DIFFUSE);
    auto cubeSpecularMap = ResourceManager::LoadTexture("resources/textures/crateSpec.jpg", TextureType::TEX_SPECULAR);

    auto grassDiffuseMap = ResourceManager::LoadTexture("resources/textures/grass.png", TextureType::TEX_DIFFUSE);

    auto windowDiffuseMap = ResourceManager::LoadTexture("resources/textures/transparent_window.png", TextureType::TEX_DIFFUSE);
    auto windowSpecMap = ResourceManager::LoadTexture("resources/textures/metalSpec.png", TextureType::TEX_SPECULAR);

    // Cubemap
    std::vector<std::string> faces = {
        "resources/textures/skybox/right.jpg",
        "resources/textures/skybox/left.jpg",
        "resources/textures/skybox/top.jpg",
        "resources/textures/skybox/bottom.jpg",
        "resources/textures/skybox/front.jpg",
        "resources/textures/skybox/back.jpg"
    };
    auto skyBox = ResourceManager::LoadCubeMap(faces);

    // Create geometry
    cube = GeometryFactory::CreateCube();
    plane = GeometryFactory::CreatePlane();
    skyMesh = GeometryFactory::CreateSkyboxCube();
    auto skyMat = std::make_shared<Material>();
    skyMat->textures["skybox"] = skyBox; 

    {
        std::shared_ptr<Entity> e = Entity::Create();
        e->AddSkybox(&skyMesh, skyMat, ResourceManager::GetShader("skybox"));
        entities.push_back(e);
    }


    {
        std::shared_ptr<Material> mat = std::make_shared<Material>();;
        mat->cullMode = CullMode::None;
        mat->isTransparent = true;

        mat->textures["material_diffuse"] = grassDiffuseMap;
        mat->floats["material_shininess"] = 16.0f;

        std::shared_ptr<Entity> e = Entity::Create();
        e->AddMesh(&plane, mat, shader);
        e->transform.position = glm::vec3(1.5f, 0.5f, 4.0f);
        e->transform.rotation = glm::vec3(270, 0, 0);
        e->transform.scale = glm::vec3(1);
        entities.push_back(e);
    }

    {
        std::shared_ptr<Material> mat = std::make_shared<Material>();;
        mat->isTransparent = true;
        mat->cullMode = CullMode::None;

        mat->textures["material_diffuse"] = windowDiffuseMap;
        mat->textures["material_specular"] = windowSpecMap;
        mat->floats["material_shininess"] = 100.0f;

        std::shared_ptr<Entity> e = Entity::Create();
        e->AddMesh(&plane, mat, shader);
        e->transform.position = glm::vec3(0.0f, 0.5f, 4.0f);
        e->transform.rotation = glm::vec3(270, 0, 0);
        e->transform.scale = glm::vec3(1);
        entities.push_back(e);
    }

    std::shared_ptr<Material> cubeMat = std::make_shared<Material>();;
    cubeMat->textures["material_diffuse"] = cubeDiffuseMap;
    cubeMat->textures["material_specular"] = cubeSpecularMap;
    cubeMat->floats["material_shininess"] =32.0f;
    cubeMat->vec3s["material_diffuseColor"] = glm::vec3(1.0f);
    cubeMat->vec3s["material_specularColor"] = glm::vec3(1.0f);
    cubeMat->showNormals = true;
    cubeMat->outlineEnabled = true;

    float spacing = 1.05f;
    float height = 1.1f;
    int base = 2;

    for (int layer = 0; layer < base; layer++)
    {
        float y = 0.55f + layer * height;
        float start = -(base - layer - 1) * (spacing / 2.0f);

        for (int i = 0; i < base - layer; i++)
            for (int j = 0; j < base - layer; j++)
            {
                std::shared_ptr<Entity> e = Entity::Create();
                e->AddMesh(&cube, cubeMat, shader);
                e->transform.position = glm::vec3(start + i * spacing, y, start + j * spacing);
                e->transform.scale = glm::vec3(1.1f);
                entities.push_back(e);
            }
    }

    {
        std::shared_ptr<Material> mat = std::make_shared<Material>();;
        mat->cullMode = CullMode::Front;
        mat->textures["material_diffuse"] = floorDiffuseMap;
        mat->textures["material_specular"] = floorSpecularMap;
        mat->floats["material_shininess"] = 32.0f;

        std::shared_ptr<Entity> e = Entity::Create();
        e->AddMesh(&plane, mat, shader);
        e->transform.scale = glm::vec3(10);
        entities.push_back(e);
    }

    lightManager->ClearPointLights();

    lightManager->SetDirectional(
        glm::vec3(-0.5f, -1.0f, -0.3f),
        glm::vec3(0.04f),
        glm::vec3(0.55f),
        glm::vec3(0.7f)
    );

    PointLight k;
    k.position = glm::vec3(1.5f, 2, 1.5f);
    k.ambient = glm::vec3(0.03f);
    k.diffuse = glm::vec3(1);
    k.specular = glm::vec3(0.5f);
    k.constant = 1; k.linear = 0.09f; k.quadratic = 0.032f;
    lightManager->AddPointLight(k);

    PointLight f;
    f.position = glm::vec3(-1, 2, 1);
    f.ambient = glm::vec3(0.04f);
    f.diffuse = glm::vec3(0.7, 0.4, 0.1);
    f.specular = glm::vec3(0.4f);
    f.constant = 1; f.linear = 0.14f; f.quadratic = 0.07f;
    lightManager->AddPointLight(f);

    PointLight r;
    r.position = glm::vec3(-1, 2, -2);
    r.ambient = glm::vec3(0.01f);
    r.diffuse = glm::vec3(0.2, 0.5, 0.4);
    r.specular = glm::vec3(0.4);
    r.constant = 1; r.linear = 0.09f; r.quadratic = 0.032f;
    lightManager->AddPointLight(r);
}

void Test::update()
{
}

void Test::render()
{
    auto app = win.GetAppState();
    if (!app) return;

    glm::mat4 view = app->camera.GetViewMatrix();
    glm::mat4 proj = glm::perspective(
        glm::radians(app->camera.Zoom),
        (float)win.Width() / win.Height(),
        0.1f, 100.0f
    );

    renderer->BeginScene(view, proj, app->camera.Position);

    // Camera-driven spotlight if you use one
    if (!lightManager->spots.empty())
    {
        lightManager->spots[0].position = app->camera.Position;
        lightManager->spots[0].direction = app->camera.Front;
    }

    // Upload global UBO (lights + camera) — replaces old ApplyToShader()
    lightManager->UploadToUBO(view, proj, app->camera.Position);

    // Draw everything
    renderer->RenderScene(entities, app->camera, lightManager, sceneFBO, postPipeline, app->wireframeEnabled);

    renderer->EndScene();
}