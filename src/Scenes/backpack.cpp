#include <thirdparty/glad/glad.h>
#include <thirdparty/stb_image.h>
#include <iostream>
#include <thirdparty/glm/gtc/matrix_transform.hpp>
#include <thirdparty/glm/gtc/type_ptr.hpp>
#include "scenes/backpack.h"
#include "core/ResourceManager.h"
#include "helpers/Utils.h"


Backpack::Backpack(Window& win)
    : shader(nullptr),
    rotationAngle(0.0f), rotationSpeed(50.0f),
    win(win), obj("resources/models/backpack/backpack.obj")
{
  
}

Backpack::~Backpack()
{
}

void Backpack::init()
{
    std::cerr << "Backpack model mesh count: " << obj.GetMeshCount() << "\n";

    // shaders
    shader = ResourceManager::LoadShader("factory",
        "shaders/modularVertexShader.vs",
        "shaders/modularFragmentShader.fs");

    entities.clear();
    Entity e;
    e.type = Entity::Type::Model;
    e.modelRenderer.model = &obj;
    e.modelRenderer.shader = shader;
    e.transform.position = glm::vec3(0.0f);
    e.transform.scale = glm::vec3(1.0f);
    entities.push_back(std::move(e));

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

void Backpack::update() {
    auto app = win.GetAppState();
    rotationAngle -= rotationSpeed * (app ? app->deltaTime : 0.016f);
    for (size_t i = 0; i < entities.size(); ++i) {
        float offset = 29.5f + 0.1f * sin(i);
        entities[i].transform.rotation.y = rotationAngle + offset * float(i);
    }
}

void Backpack::render()
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