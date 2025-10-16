#pragma once
#include <glm/glm.hpp>
#include <memory>
#include <iostream>
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
    Mesh* mesh = nullptr;                              // pointer to shared mesh
    std::shared_ptr<Shader> shader;                    // material shader
    std::shared_ptr<Material> material = nullptr;      // optional material
};

struct ModelRenderer
{
    Model* model = nullptr;                            // pointer to model
    std::shared_ptr<Shader> shader;
};

struct Entity
{
    Transform transform;

    enum class Type { None, Mesh, Model } type = Type::None;

    MeshRenderer meshRenderer;
    ModelRenderer modelRenderer;

    void Render(Renderer& renderer)
    {
        glm::mat4 modelMatrix = transform.GetModelMatrix();

        // --- Default material (created once) ---
        static std::shared_ptr<Material> defaultMaterial = []() {
            auto mat = std::make_shared<Material>();
            mat->useDiffuseMap = false;
            mat->useSpecularMap = false;
            mat->diffuseColor = glm::vec3(1.0f);      // pure white
            mat->specularColor = glm::vec3(0.04f);    // subtle specular
            mat->shininess = 16.0f;
            return mat;
            }();

        switch (type)
        {
        case Type::Mesh:
            if (meshRenderer.mesh && meshRenderer.shader)
            {
                // fallback if material is missing
                std::shared_ptr<Material> mat =
                    (meshRenderer.material ? meshRenderer.material : defaultMaterial);

                renderer.SubmitMesh(modelMatrix,
                    *meshRenderer.mesh,
                    meshRenderer.shader,
                    mat);
            }
            break;

        case Type::Model:
            if (modelRenderer.model && modelRenderer.shader)
            {
                renderer.SubmitModel(modelMatrix,
                    *modelRenderer.model,
                    modelRenderer.shader);
            }
            break;

        default:
            break;
        }
    }
};