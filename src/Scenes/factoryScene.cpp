#include <thirdparty/glad/glad.h>
#include <thirdparty/stb_image.h>
#include <iostream>
#include <thirdparty/glm/gtc/matrix_transform.hpp>
#include <thirdparty/glm/gtc/type_ptr.hpp>
#include "scenes/factoryScene.h"
#include "core/ResourceManager.h"


FactoryScene::FactoryScene(Window& win)
    : shader(nullptr),
    rotationAngle(0.0f), rotationSpeed(50.0f),
    win(win)
{
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

void FactoryScene::init() {
    // shaders
    shader = ResourceManager::LoadShader("factory", 
        "shaders/lightning/combinedLightVertexShader.vs", 
        "shaders/lightning/combinedLightsFragmentShader.fs");

    diffuseMap = ResourceManager::LoadTexture(
            "container_diff", "resources/container3.png");
    specularMap = ResourceManager::LoadTexture(
            "container_spec", "resources/container3Spec.png");

    cubeMesh = Mesh::CreateCube();

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

void FactoryScene::update() {
    rotationAngle -= rotationSpeed * win.GetAppState() -> deltaTime;
}

void FactoryScene::render() 
{
    glm::vec3 pointLightPositions[] = {
        glm::vec3(0.7f, 0.2f, 2.0f),
        glm::vec3(2.3f, -3.3f, -4.0f),
        glm::vec3(-4.0f, 2.0f, -12.0f),
        glm::vec3(0.0f, 0.0f, -3.0f)
    };

    shader->use();
    glm::mat4 view = win.GetAppState()->camera.GetViewMatrix();
    glm::mat4 proj = glm::perspective(glm::radians(win.GetAppState()->camera.Zoom),
        (float)win.Width() / (float)win.Height(), 0.1f, 100.0f);

    renderer.BeginScene(view, proj, win.GetAppState()->camera.Position);

    if (!lightManager.spots.empty()) {
        lightManager.spots[0].position = win.GetAppState()->camera.Position;
        lightManager.spots[0].direction = win.GetAppState()->camera.Front;
    }
    if (shader) lightManager.ApplyToShader(*shader);

    glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, diffuseMap);
    glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, specularMap);

    glBindVertexArray(cubeMesh.VAO);
    for (int i = 0; i < 10; i++) {
        glm::mat4 model(1.0f);
        model = glm::translate(model, cubePositions[i]);
        float angle = 20.0f * i;
        model = glm::rotate(model, glm::radians(angle) + glm::radians(rotationAngle), glm::vec3(1.0f, 0.3f, 0.5f));
        renderer.Submit(model, cubeMesh, shader, diffuseMap, specularMap, 32.0f);
    }
}