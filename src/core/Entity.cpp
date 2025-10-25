#include "core/Entity.h"
#include <glm/gtc/matrix_transform.hpp>

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

glm::mat4 Transform::GetModelMatrix() const 
{
    glm::mat4 m(1.0f);
    m = glm::translate(m, position);
    m = glm::rotate(m, glm::radians(rotation.x), glm::vec3(1, 0, 0));
    m = glm::rotate(m, glm::radians(rotation.y), glm::vec3(0, 1, 0));
    m = glm::rotate(m, glm::radians(rotation.z), glm::vec3(0, 0, 1));
    m = glm::scale(m, scale);
    return m;
}


void Entity::Render(Renderer& renderer)
{
    glm::mat4 modelMatrix = transform.GetModelMatrix();

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

    case Type::SkyBox:
        renderer.SubmitSkybox(this);
        break;

    default:
        break;
    }
}