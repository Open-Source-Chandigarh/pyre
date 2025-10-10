#pragma once
#include <glm/glm.hpp>
#include <memory>
#include "core/rendering/Mesh.h"
#include "core/rendering/Model.h"
#include "helpers/shaderClass.h"
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
    Mesh* mesh = nullptr;               // pointer to shared mesh
    std::shared_ptr<Shader> shader;
    unsigned int diffuse = 0;
    unsigned int specular = 0;
    float shininess = 32.0f;
};

struct ModelRenderer
{
    Model* model = nullptr;             // pointer to model
    std::shared_ptr<Shader> shader;
};

struct Entity
{
    Transform transform;

    // Type of renderer used
    enum class Type { None, Mesh, Model } type = Type::None;

    // Always present
    MeshRenderer meshRenderer;
    ModelRenderer modelRenderer;

    // Render function
    void Render(Renderer& renderer)
    {
        glm::mat4 modelMatrix = transform.GetModelMatrix();

        switch (type)
        {
        case Type::Mesh:
            if (meshRenderer.mesh && meshRenderer.shader)
                renderer.SubmitMesh(modelMatrix,
                    *meshRenderer.mesh,
                    meshRenderer.shader,
                    meshRenderer.diffuse,
                    meshRenderer.specular,
                    meshRenderer.shininess);
            break;

        case Type::Model:
            if (modelRenderer.model && modelRenderer.shader)
                renderer.SubmitModel(modelMatrix, *modelRenderer.model,
                    modelRenderer.shader);
            break;

        default:
            break;
        }
    }
};