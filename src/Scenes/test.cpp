#include "scenes/test.h"
#include "core/ResourceManager.h"
#include "core/rendering/geometry/GeometryFactory.h"

Test::Test(Window& win) : win(win), shader(nullptr)
{
}

void Test::init()
{
    shader = ResourceManager::LoadShader("test",
        "shaders/modularVertexShader.vs",
        "shaders/modularFragmentShader.fs");

    floorDiffuseMap = ResourceManager::LoadTexture("resources/textures/woodDiff.png", TextureType::TEX_DIFFUSE);
    floorSpecularMap = ResourceManager::LoadTexture("resources/textures/woodSpec.png", TextureType::TEX_SPECULAR);

    cubeDiffuseMap = ResourceManager::LoadTexture("resources/textures/crateDiff.jpg", TextureType::TEX_DIFFUSE);
    cubeSpecularMap = ResourceManager::LoadTexture("resources/textures/crateSpec.jpg", TextureType::TEX_SPECULAR);

    std::shared_ptr<Texture> grassDiffuseMap =
        ResourceManager::LoadTexture("resources/textures/grass.png", TextureType::TEX_DIFFUSE);

    std::shared_ptr<Texture> windowDiffuseMap =
        ResourceManager::LoadTexture("resources/textures/transparent_window.png", TextureType::TEX_DIFFUSE);
    std::shared_ptr<Texture> windowSpecMap =
        ResourceManager::LoadTexture("resources/textures/metalSpec.png", TextureType::TEX_SPECULAR);

    // create procedural geometry
    cube = GeometryFactory::CreateCube();
    plane = GeometryFactory::CreatePlane();

    Material grassMat;
    grassMat.useDiffuseMap = true;
    grassMat.useSpecularMap = false;
    grassMat.cullMode = CullMode::None;
    grassMat.specularColor = glm::vec3(0.0f);
    grassMat.textures.push_back(grassDiffuseMap);

    Material windowMat;
    windowMat.useDiffuseMap = true;
    windowMat.useSpecularMap = true;
    windowMat.isTransparent = true;
    windowMat.cullMode = CullMode::None;
    windowMat.textures.push_back(windowDiffuseMap);
    windowMat.textures.push_back(windowSpecMap);

    Entity* windowEntity = new Entity();
    windowEntity->type = Entity::Type::Mesh;
    windowEntity->meshRenderer.mesh = &plane;
    windowEntity->meshRenderer.material = std::make_shared<Material>(windowMat);
    windowEntity->meshRenderer.shader = shader;
    windowEntity->transform.position = glm::vec3(0.0f, 0.0f, 2.0f);
    windowEntity->transform.rotation = glm::vec3(270.0f, 0.0f, 0.0f);
    windowEntity->transform.position.y += 0.5f * windowEntity->transform.scale.y;
    windowEntity->transform.scale = glm::vec3(1.0f);
    entities.push_back(windowEntity);

    Entity* grassEntity = new Entity();
    grassEntity->type = Entity::Type::Mesh;
    grassEntity->meshRenderer.mesh = &plane;
    grassEntity->meshRenderer.material = std::make_shared<Material>(grassMat);
    grassEntity->meshRenderer.shader = shader;
    grassEntity->transform.position = glm::vec3(1.5f, 0.0f, 2.0f);
    grassEntity->transform.rotation = glm::vec3(270.0f, 0.0f, 0.0f);
    grassEntity->transform.position.y += 0.5f * grassEntity->transform.scale.y;
    grassEntity->transform.scale = glm::vec3(1.0f);
    entities.push_back(grassEntity);

    // Cube 
    Material cubeMat;
    cubeMat.useDiffuseMap = true;
    cubeMat.useSpecularMap = true;
    cubeMat.outlineEnabled = true;
    cubeMat.shininess = 96.0f;
    cubeMat.textures.push_back(cubeDiffuseMap);
    cubeMat.textures.push_back(cubeSpecularMap);

    // Floor
    Material floorMat;
    floorMat.useDiffuseMap = true;
    floorMat.useSpecularMap = true;
    floorMat.cullMode = CullMode::Front;
    floorMat.shininess = 10.0f;
    floorMat.textures.push_back(floorDiffuseMap);
    floorMat.textures.push_back(floorSpecularMap);

    float cubeSpacing = 1.05f;
    float cubeHeight = 1.1f;
    int baseCount = 2;

    for (int layer = 0; layer < baseCount; ++layer)
    {
        int cubesPerRow = baseCount - layer;
        float y = 0.55f + (layer * cubeHeight);
        float startOffset = -(cubesPerRow - 1) * (cubeSpacing / 2.0f);

        for (int i = 0; i < cubesPerRow; ++i)
        {
            for (int j = 0; j < cubesPerRow; ++j)
            {
                Entity* cubeEntity = new Entity();
                cubeEntity->type = Entity::Type::Mesh;
                cubeEntity->meshRenderer.mesh = &cube;
                cubeEntity->meshRenderer.material = std::make_shared<Material>(cubeMat);
                cubeEntity->meshRenderer.shader = shader;

                float x = startOffset + i * cubeSpacing;
                float z = startOffset + j * cubeSpacing;

                cubeEntity->transform.position = glm::vec3(x, y, z);
                cubeEntity->transform.rotation = glm::vec3(0.0f);
                cubeEntity->transform.scale = glm::vec3(1.1f);

                entities.push_back(cubeEntity);
            }
        }
    }

    Entity* eFloor = new Entity();
    eFloor->type = Entity::Type::Mesh;
    eFloor->meshRenderer.mesh = &plane;
    eFloor->meshRenderer.material = std::make_shared<Material>(floorMat);
    eFloor->meshRenderer.shader = shader;
    eFloor->transform.position = glm::vec3(0.0f);
    eFloor->transform.scale = glm::vec3(10.0f);
    entities.push_back(eFloor);

    // lighting setup
    lightManager.ClearPointLights();
    lightManager.SetDirectional(
        glm::vec3(-0.5f, -1.0f, -0.3f),
        glm::vec3(0.04f),
        glm::vec3(0.55f),
        glm::vec3(0.7f)
    );

    PointLight key;
    key.position = glm::vec3(1.5f, 2.0f, 1.5f);
    key.ambient = glm::vec3(0.03f);
    key.diffuse = glm::vec3(1.0f);
    key.specular = glm::vec3(1.0f);
    key.constant = 1.0f; key.linear = 0.09f; key.quadratic = 0.032f;
    lightManager.AddPointLight(key);

    PointLight fill;
    fill.position = glm::vec3(-1.0f, 0.7f, 0.8f);
    fill.ambient = glm::vec3(0.02f);
    fill.diffuse = glm::vec3(0.25f);
    fill.specular = glm::vec3(0.2f);
    fill.constant = 1.0f; fill.linear = 0.14f; fill.quadratic = 0.07f;
    lightManager.AddPointLight(fill);

    PointLight rim;
    rim.position = glm::vec3(-1.0f, 1.5f, -1.5f);
    rim.ambient = glm::vec3(0.0f);
    rim.diffuse = glm::vec3(0.15f, 0.18f, 0.22f);
    rim.specular = glm::vec3(0.4f);
    rim.constant = 1.0f; rim.linear = 0.09f; rim.quadratic = 0.032f;
    lightManager.AddPointLight(rim);
}

void Test::update()
{

}

void Test::render()
{
    auto app = win.GetAppState();
    if (!app) return;

    glm::mat4 view = app->camera.GetViewMatrix();
    glm::mat4 proj = glm::perspective(glm::radians(app->camera.Zoom),
        (float)win.Width() / (float)win.Height(), 0.1f, 100.0f);

    renderer.BeginScene(view, proj, app->camera.Position);

    if (!lightManager.spots.empty()) {
        lightManager.spots[0].position = win.GetAppState()->camera.Position;
        lightManager.spots[0].direction = win.GetAppState()->camera.Front;
    }

    if (shader) lightManager.ApplyToShader(*shader);

    // Draw entities
    renderer.RenderScene(entities, app->camera);
    renderer.EndScene();
}