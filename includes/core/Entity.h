#pragma once
#include <glm/glm.hpp>
#include <memory>
#include <iostream>
#include "core/rendering/Mesh.h"
#include "core/rendering/Model.h"
#include "helpers/Shader.h"
#include "core/rendering/Renderer.h"

struct Transform
{
    glm::vec3 position{ 0.0f };
    glm::vec3 scale{ 1.0f };
    glm::vec3 rotation{ 0.0f }; // Euler angles yaw pitch roll

    glm::mat4 GetModelMatrix() const;
};

struct MeshRenderer
{
    Mesh* mesh = nullptr;                              // pointer to shared mesh
    std::shared_ptr<Shader> shader;                    // material shader
    std::shared_ptr<Material> material = nullptr;      // optional material
};

struct RenderSettings
{
    bool showNormals = false;
    bool outlineEnabled = false;
    glm::vec3 outlineColor = glm::vec3(1.0f, 1.0f, 1.0f);
};

struct ModelRenderer
{
    Model* model = nullptr;                            // pointer to model
    std::shared_ptr<Shader> shader;
    RenderSettings settings;
    // Count (Default 1 = Normal, >1 = Instanced)
    int instanceCount = 1; 
};

struct Entity : public std::enable_shared_from_this<Entity>
{
    // only way to create an Entity
    static std::shared_ptr<Entity> Create() 
    {
        return std::shared_ptr<Entity>(new Entity());
    }

    Transform transform;

    enum class Type { None, Mesh, Model, SkyBox } type = Type::None;

    MeshRenderer meshRenderer;
    ModelRenderer modelRenderer;

    void Render(Renderer& renderer);

private:
    // PRIVATE CONSTRUCTOR
    // This prevents "Entity e;" (stack) and "std::make_shared<Entity>()" (external)
    Entity() = default;
};