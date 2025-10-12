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

    // create procedural geometry
    cube = GeometryFactory::CreateCube();
    floor = GeometryFactory::CreatePlane(5.0f);

    // Cube (red plastic)
    Material cubeMat;
    cubeMat.useDiffuseMap = true;
    cubeMat.useSpecularMap = true;
    cubeMat.diffuseColor = glm::vec3(0.8f, 0.05f, 0.05f);    // slightly warm red
    cubeMat.specularColor = glm::vec3(0.95f, 0.95f, 0.95f);  // very bright specular for plastic
    cubeMat.shininess = 96.0f;   
    
    cubeMat.textures.push_back(cubeDiffuseMap);
    cubeMat.textures.push_back(cubeSpecularMap);

    // Floor (less shiny, grounded)
    Material floorMat;
    floorMat.useDiffuseMap = true;
    floorMat.useSpecularMap = true;
    floorMat.diffuseColor = glm::vec3(1.0f);
    floorMat.specularColor = glm::vec3(0.2f);  // low specular for rough wood
    floorMat.shininess = 16.0f;                // broad, soft highlights

    floorMat.textures.push_back(floorDiffuseMap);
    floorMat.textures.push_back(floorSpecularMap);


    // --- create entities that reference the mesh instances ---
    Entity cube1;
    cube1.type = Entity::Type::Mesh;
    cube1.meshRenderer.mesh = &cube;         // points to the mesh with material
    cube1.meshRenderer.shader = shader;
    cube1.meshRenderer.material = std::make_shared<Material>(cubeMat);
    cube1.transform.position = glm::vec3(0.0f, 0.5f, 0.0f);
    cube1.transform.scale = glm::vec3(1.0f);
    entities.push_back(cube1);

    Entity cube2;
    cube2.type = Entity::Type::Mesh;
    cube2.meshRenderer.mesh = &cube;         // re-uses same mesh/material
    cube2.meshRenderer.material = std::make_shared<Material>(cubeMat);
    cube2.meshRenderer.shader = shader;
    cube2.transform.position = glm::vec3(0.6f, 0.5f, 2.0f);
    cube2.transform.scale = glm::vec3(1.0f);
    entities.push_back(cube2);

    Entity eFloor;
    eFloor.type = Entity::Type::Mesh;
    eFloor.meshRenderer.mesh = &floor;       // plane has its own material
    eFloor.meshRenderer.material = std::make_shared<Material>(floorMat);
    eFloor.meshRenderer.shader = shader;
    eFloor.transform.position = glm::vec3(0.0f, 0.0f, 0.0f);
    eFloor.transform.scale = glm::vec3(1.5f);
    entities.push_back(eFloor);

    // --- lighting: directional (global) ---
    lightManager.ClearPointLights();
    lightManager.SetDirectional(
        glm::vec3(-0.5f, -1.0f, -0.3f),   // angled down & left
        glm::vec3(0.04f),                  // small ambient
        glm::vec3(0.55f, 0.55f, 0.55f),    // nice diffuse
        glm::vec3(0.7f, 0.7f, 0.7f)        // specular
    );

    // --- primary key point light: strong highlight ---
    PointLight key;
    key.position = glm::vec3(1.5f, 2.0f, 1.5f);   // above & front-right of cube
    key.ambient = glm::vec3(0.03f);               // tiny ambient
    key.diffuse = glm::vec3(1.0f);                // white diffuse for color pop
    key.specular = glm::vec3(1.0f);               // white specular for tight highlights
    key.constant = 1.0f; key.linear = 0.09f; key.quadratic = 0.032f;
    lightManager.AddPointLight(key);

    // --- fill point light: subtle, opposite side to soften shadows ---
    PointLight fill;
    fill.position = glm::vec3(-1.0f, 0.7f, 0.8f); // left, slightly low
    fill.ambient = glm::vec3(0.02f);
    fill.diffuse = glm::vec3(0.25f);              // much weaker
    fill.specular = glm::vec3(0.2f);
    fill.constant = 1.0f; fill.linear = 0.14f; fill.quadratic = 0.07f;
    lightManager.AddPointLight(fill);

    // --- rim/back light: small cool edge ---
    PointLight rim;
    rim.position = glm::vec3(-1.0f, 1.5f, -1.5f); // behind cube
    rim.ambient = glm::vec3(0.0f);
    rim.diffuse = glm::vec3(0.15f, 0.18f, 0.22f); // subtle bluish rim
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
    for (auto& e : entities) {
        e.Render(renderer);
    }
    renderer.EndScene();
}