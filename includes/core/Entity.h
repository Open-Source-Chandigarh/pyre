#pragma once
#include <memory>
#include <string>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "core/rendering/Renderer.h"

class Mesh;
class Model;
class Shader;
struct Material;

struct Transform 
{
    glm::vec3 position{ 0.0f };
    glm::vec3 scale{ 1.0f };
    glm::vec3 rotation{ 0.0f }; // Euler angles in degrees

    glm::mat4 GetModelMatrix() const 
    {
        glm::mat4 m(1.0f);
        m = glm::translate(m, position);
        m = glm::rotate(m, glm::radians(rotation.x), glm::vec3(1, 0, 0));
        m = glm::rotate(m, glm::radians(rotation.y), glm::vec3(0, 1, 0));
        m = glm::rotate(m, glm::radians(rotation.z), glm::vec3(0, 0, 1));
        m = glm::scale(m, scale);
        return m;
    }
};

struct MeshComponent 
{
    Mesh* mesh = nullptr;             
    std::shared_ptr<Material> material; 
    std::shared_ptr<Shader> shader;   
};

struct ModelComponent 
{
    Model* model = nullptr; 
    std::shared_ptr<Shader> shader;
    int instanceCount = 1;
    RenderSettings renderSettings;
};

struct SkyboxComponent 
{
    Mesh* mesh = nullptr;
    std::shared_ptr<Material> material; // Should contain the Cubemap texture
    std::shared_ptr<Shader> shader;
};

struct Entity 
{
    std::string name;
    Transform transform;

    // Component Slots 
    std::shared_ptr<MeshComponent> meshComp;
    std::shared_ptr<ModelComponent> modelComp;
    std::shared_ptr<SkyboxComponent> skyboxComp;

    //Helper Factory
    static std::shared_ptr<Entity> Create(const std::string& name = "Entity") {
        auto e = std::make_shared<Entity>();
        e->name = name;
        return e;
    }

    //Helper Methods to Add Components
    
    void AddMesh(Mesh* mesh, std::shared_ptr<Material> mat, std::shared_ptr<Shader> shader) 
    {
        meshComp = std::make_shared<MeshComponent>();
        meshComp->mesh = mesh;
        meshComp->material = mat;
        meshComp->shader = shader;
    }

    void AddModel(Model* model, std::shared_ptr<Shader> shader, int instanceCount = 1,
                bool showNormals = false, bool showOutline = false, 
                glm::vec3 outlineColor = glm::vec3(1.0, 1.0, 1.0), bool castsShadows = true)
     {
        modelComp = std::make_shared<ModelComponent>();
        modelComp->model = model;
        modelComp->shader = shader;
        modelComp->instanceCount = instanceCount;
        modelComp->renderSettings.showNormals = showNormals;
        modelComp->renderSettings.outlineColor = outlineColor;
        modelComp->renderSettings.outlineEnabled = showOutline;
        modelComp->renderSettings.castsShadows = castsShadows;
    }
    
    void AddSkybox(Mesh* mesh, std::shared_ptr<Material> mat, std::shared_ptr<Shader> shader) 
    {
        skyboxComp = std::make_shared<SkyboxComponent>();
        skyboxComp->mesh = mesh;
        skyboxComp->material = mat;
        skyboxComp->shader = shader;
    }
};