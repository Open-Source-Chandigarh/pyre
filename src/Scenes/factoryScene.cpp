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
    renderer = std::make_shared<Renderer>();
    lightManager = std::make_shared<LightManager>();

    // Create scene framebuffer once
    sceneFBO = std::make_shared<Framebuffer>((unsigned int)win.Width(), 
        (unsigned int)win.Height(), true, true);

    // Create post processing pipeline and add effects
    postPipeline = std::make_shared<PostProcessingPipeline>((unsigned int)win.Width(), 
        (unsigned int)win.Height());
    // postPipeline->AddInversion();
    // postPipeline->AddGrayscale();
    // postPipeline->AddSharpen(5.0f);
    postPipeline->AddGammaCorrection(2.2f);

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

    std::shared_ptr<Entity> skyEntity = Entity::Create();
    skyEntity->type = Entity::Type::SkyBox;
    skyEntity->meshRenderer.mesh = &skyMesh;
    skyEntity->meshRenderer.shader = ResourceManager::GetShader("skybox");

    // store cubemap texture in material
    std::shared_ptr<Material> skyMat = std::make_shared<Material>();
    skyMat->textures["skybox"] = skyBox;
    skyEntity->meshRenderer.material = skyMat;
    entities.push_back(std::move(skyEntity));

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
            mesh[i] = GeometryFactory::CreateCube();
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
        mat -> textures["material_skybox"] = skyBox;
       if (randomInt == 0) { 
             mat->floats["material_shininess"] = 128.0f; 
             mat->floats["material_reflectivity"] = 0.8f; 
        }
        else if (randomInt == 1) { 
             mat->floats["material_shininess"] = 16.0f; 
             mat->floats["material_reflectivity"] = 0.1f; 
        }
        else { 
             mat->floats["material_shininess"] = 64.0f;
             mat->floats["material_reflectivity"] = 0.3f;
        }

        std::shared_ptr<Entity> e = Entity::Create();
        e->type = Entity::Type::Mesh;
        e->meshRenderer.mesh = &mesh[i];
        e->meshRenderer.material = mat;
        e->meshRenderer.shader = shader;
        e->transform.position = cubePositions[i];
        e->transform.scale = glm::vec3(0.7f);
        entities.push_back(std::move(e));
    }

   lightManager->ClearPointLights();

    lightManager->SetDirectional(glm::vec3(-0.5f, -1.0f, -0.5f),
        glm::vec3(0.05f), glm::vec3(0.1f, 0.15f, 0.3f), glm::vec3(0.2f));

    PointLight keyLight;
    keyLight.position = glm::vec3(0.0f, 2.0f, 2.0f);
    keyLight.ambient = glm::vec3(0.0f); 
    keyLight.diffuse = glm::vec3(1.0f, 0.6f, 0.3f) * 1.5f; 
    keyLight.specular = glm::vec3(1.0f, 0.8f, 0.6f); 
    keyLight.constant = 1.0f; keyLight.linear = 0.09f; keyLight.quadratic = 0.032f;
    lightManager->AddPointLight(keyLight);


    PointLight rimLight;
    rimLight.position = glm::vec3(-3.0f, 1.0f, -5.0f);
    rimLight.ambient = glm::vec3(0.0f);
    rimLight.diffuse = glm::vec3(0.0f, 0.5f, 1.0f) * 1.0f; 
    rimLight.specular = glm::vec3(0.0f, 1.0f, 1.0f);
    rimLight.constant = 1.0f; rimLight.linear = 0.09f; rimLight.quadratic = 0.032f;
    lightManager->AddPointLight(rimLight);
}

void FactoryScene::update() 
{
  auto app = win.GetAppState();
    float time = (float)glfwGetTime();

    for (size_t i = 0; i < entities.size(); ++i) {
        if(entities[i]->type == Entity::Type::SkyBox) continue;

        float yOffset = sin(time * 0.5f + i) * 0.5f; 
    
        entities[i]->transform.position = cubePositions[i]; 
        entities[i]->transform.position.y += yOffset;
        entities[i]->transform.rotation.x = time * 5.0f * (i % 2 == 0 ? 1 : -1);
        entities[i]->transform.rotation.y = time * 3.0f;
    }
}

void FactoryScene::render()
{
    auto app = win.GetAppState();
    if (!app) return;

    glm::mat4 view = app->camera.GetViewMatrix();
    glm::mat4 proj = glm::perspective(glm::radians(app->camera.Zoom),
        (float)win.Width() / (float)win.Height(), 0.1f, 100.0f);

    renderer->BeginScene(view, proj, app->camera.Position);

    if (!lightManager->spots.empty()) {
        lightManager->spots[0].position = win.GetAppState()->camera.Position;
        lightManager->spots[0].direction = win.GetAppState()->camera.Front;
    }

    //if (shader) lightManager.ApplyToShader(*shader, renderer, view, proj);

    lightManager->UploadToUBO(view, proj, app->camera.Position);

    renderer->RenderScene(entities, app->camera, nullptr, sceneFBO, postPipeline, app->wireframeEnabled);

    renderer->EndScene();
    Framebuffer::Unbind();
}

void FactoryScene::OnResize(int w, int h) 
{
    if (sceneFBO) sceneFBO->Resize((unsigned int)w, (unsigned int)h);
    if (postPipeline) postPipeline->Resize((unsigned int)w, (unsigned int)h);
}