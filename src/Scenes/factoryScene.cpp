#include <thirdparty/glad/glad.h>
#include <thirdparty/stb_image.h>
#include <iostream>
#include <thirdparty/glm/gtc/matrix_transform.hpp>
#include <thirdparty/glm/gtc/type_ptr.hpp>
#include "scenes/factoryScene.h"
#include "core/ResourceManager.h"
#include "helpers/Utils.h"
#include "core/postprocessing/GenericPostEffect.h"
#include "core/rendering/geometry/GeometryFactory.h"
#include "core/rendering/GlobalUBO.h"


FactoryScene::FactoryScene(Window& win)
    : shader(nullptr),
    rotationAngle(0.0f), rotationSpeed(50.0f),
    win(win)
{

    // Create scene framebuffer once
    sceneFBO = std::make_unique<Framebuffer>((unsigned int)win.Width(), 
        (unsigned int)win.Height(), true);

    // Create post processing pipeline and add effects
    postPipeline = std::make_unique<PostProcessingPipeline>((unsigned int)win.Width(), 
        (unsigned int)win.Height());
    // postPipeline->AddInversion();
    //postPipeline->AddGrayscale();
    // postPipeline->AddSharpen(10.0f);

    // cube positions
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

FactoryScene::~FactoryScene()
{
}

void FactoryScene::init()
{
    CreateGlobalUBO();
    // shaders
    shader = ResourceManager::LoadShader("factory",
        "shaders/modularVertexShader.vs",
        "shaders/modularFragmentShader.fs");

    diffuseMap = ResourceManager::LoadTexture(
        "resources/textures/metalDiff.png", TextureType::TEX_DIFFUSE);
    specularMap = ResourceManager::LoadTexture(
        "resources/textures/metalSpec.png", TextureType::TEX_SPECULAR);

    ResourceManager::LoadShader("skybox",
        "shaders/skyBox.vs", "shaders/skyBox.fs");

    std::vector<std::string> faces = {
        "resources/textures/stylizedSky/front.png",
          "resources/textures/stylizedSky/back.png",
        "resources/textures/stylizedSky/top.png",
        "resources/textures/stylizedSky/bottom.png",
        "resources/textures/stylizedSky/right.png",
        "resources/textures/stylizedSky/left.png",
    };
    std::shared_ptr<Texture> skyBox =
        ResourceManager::LoadCubeMap(faces);
    skyMesh = GeometryFactory::CreateSkyboxCube(); // position-only mesh (36 verts)

    Entity* skyEntity = new Entity();
    skyEntity->type = Entity::Type::SkyBox;
    skyEntity->meshRenderer.mesh = &skyMesh;
    skyEntity->meshRenderer.shader = ResourceManager::GetShader("skybox");

    // store cubemap texture in material
    std::shared_ptr<Material> skyMat = std::make_shared<Material>();
    skyMat->textures["skybox"] = skyBox;
    skyEntity->meshRenderer.material = skyMat;
    entities.push_back(std::move(skyEntity));

    // --- create meshes first and give each its own Material ---
    for (int i = 0; i < 10; ++i)
    {
        int randomInt = Utils::RandomInt(0, 4);
        if (randomInt == 0)
            mesh[i] = GeometryFactory::CreateSphere();
        else if (randomInt == 1)
            mesh[i] = GeometryFactory::CreateCube();
        else if (randomInt == 2)
            mesh[i] = GeometryFactory::CreateTorus();
        else if (randomInt == 3)
            mesh[i] = GeometryFactory::CreateCylinder();
        else /* randomInt == 4 */
            mesh[i] = GeometryFactory::CreateCone();
    }

    for (int i = 0; i < 10; ++i)
    {
        int randomInt = Utils::RandomInt(0, 4);
        // Construct a Material for this mesh
        std::shared_ptr<Material> mat = std::make_shared<Material>();
        mat -> textures["material_diffuse"] = diffuseMap;
        mat -> textures["material_specular"] = specularMap;
        mat -> floats["material_shininess"] = 5.0f;

        // Optionally tweak material per primitive type for visual variety
        if (randomInt == 0) { mat->floats["material_shininess"] = 3.0f; } // sphere - glossier
        if (randomInt == 2) { mat->floats["material_shininess"] = 10.0f; } // torus - slightly rougher
        Entity* e = new Entity();
        e->type = Entity::Type::Mesh;
        e->meshRenderer.mesh = &mesh[i];
        e->meshRenderer.material = mat;
        e->meshRenderer.shader = shader;
        e->transform.position = cubePositions[i];
        e->transform.scale = glm::vec3(0.7f);
        entities.push_back(std::move(e));
    }

    // lights
    glm::vec3 lightColor(0.2f, 0.4f, 0.8f);
    lightManager.ClearPointLights();
    lightManager.SetDirectional(glm::vec3(-0.2f, -1.0f, -0.3f),
        glm::vec3(0.05f), glm::vec3(0.1f, 0.1f, 0.8f), glm::vec3(0.5f));

    PointLight p;
    p.position = glm::vec3(0.7f, 0.2f, 2.0f);
    p.ambient = glm::vec3(0.2f, 1.0f, 1.5f) * 0.1f;
    p.diffuse = glm::vec3(0.2f, 1.5f, 1.5f) * 0.8f;
    p.specular = glm::vec3(1.0f);
    p.constant = 1.0f; p.linear = 0.09f; p.quadratic = 0.032f;
    lightManager.AddPointLight(p);

    PointLight p2;
    p2.position = glm::vec3(2.3f, -3.3f, -4.0f);
    p2.ambient = glm::vec3(1.5f, 1.0f, 2.0f) * 0.1f;
    p2.diffuse = glm::vec3(2.0f, 1.5f, 2.0f) * 0.9f;
    p2.specular = glm::vec3(1.0f);
    p2.constant = 1.0f; p2.linear = 0.09f; p2.quadratic = 0.032f;
    lightManager.AddPointLight(p2);

    PointLight p3;
    p3.position = glm::vec3(-4.0f, 2.0f, -12.0f);
    p3.ambient = glm::vec3(1.0f, 5.0f, 0.6f) * 0.1f;
    p3.diffuse = glm::vec3(1.0f, 7.0f, 0.6f) * 0.8f;
    p3.specular = glm::vec3(1.0f);
    p3.constant = 1.0f; p3.linear = 0.09f; p3.quadratic = 0.032f;
    lightManager.AddPointLight(p3);

    PointLight p4;
    p4.position = glm::vec3(0.0f, 0.0f, -3.0f);
    p4.ambient = glm::vec3(2.0f, 1.0f, 3.0f) * 0.1f;
    p4.diffuse = glm::vec3(3.0f, 1.5f, 5.0f) * 0.6f;
    p4.specular = glm::vec3(1.0f);
    p4.constant = 1.0f; p4.linear = 0.09f; p4.quadratic = 0.032f;
    lightManager.AddPointLight(p4);

    // Spot light
    SpotLight s;
    s.position = win.GetAppState()->camera.Position;
    s.direction = win.GetAppState()->camera.Front;
    s.ambient = lightColor * 0.5f;
    s.diffuse = lightColor * 6.0f;
    s.specular = glm::vec3(1.0f);
    s.constant = 1.0f;
    s.quadratic = 0.09f;
    s.linear = 0.032f;
    s.innerCutOff = cos(glm::radians(12.5f));
    s.outerCutOff = cos(glm::radians(17.5f));
    lightManager.AddSpotLight(s);
}

void FactoryScene::update() 
{
    auto app = win.GetAppState();
    rotationAngle -= rotationSpeed * (app ? app->deltaTime : 0.016f);
    for (size_t i = 0; i < entities.size(); ++i) {
        float offset = 29.5f + 0.1f * sin(i);
        entities[i]->transform.rotation.x = rotationAngle + offset * float(i);
        entities[i]->transform.rotation.y = rotationAngle + offset * float(i);
    }
}

void FactoryScene::render()
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

    //if (shader) lightManager.ApplyToShader(*shader, renderer, view, proj);

    lightManager.UploadToUBO(view, proj, app->camera.Position);

    renderer.RenderScene(entities, app->camera, sceneFBO.get(), postPipeline.get(), app->wireframeEnabled);

    renderer.EndScene();
    Framebuffer::Unbind();
}

void FactoryScene::OnResize(int w, int h) 
{
    if (sceneFBO) sceneFBO->Resize((unsigned int)w, (unsigned int)h);
    if (postPipeline) postPipeline->Resize((unsigned int)w, (unsigned int)h);
}