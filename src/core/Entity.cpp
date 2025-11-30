#include "core/Entity.h"
#include <glm/gtc/matrix_transform.hpp>

// --- Default material (created once) ---
static std::shared_ptr<Material> defaultMaterial = []() {
    auto mat = std::make_shared<Material>();

    // Use built-in defaults (these are used by Material::ApplyToShader fallback logic)
    mat->defaultDiffuseColor = glm::vec3(1.0f);    // white
    mat->defaultShininess = 16.0f;

    // Also provide uniform-style entries so shaders that check for material_diffuseColor get a value
    mat->vec3s["material_diffuseColor"] = mat->defaultDiffuseColor;
    mat->floats["material_shininess"] = mat->defaultShininess;

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
            if (modelRenderer.instanceCount > 1)
            {
                // Instanced Draw (Position/Scale in 'transform' is IGNORED here)
                // The transforms come from the Mesh VBO
                renderer.SubmitInstancedModel(
                    *modelRenderer.model, 
                    modelRenderer.shader, 
                    modelRenderer.instanceCount
                );
            }
            else
            {
                // Standard Single Draw
                renderer.SubmitModel(
                    modelMatrix, 
                    *modelRenderer.model, 
                    modelRenderer.shader, 
                    modelRenderer.settings
                );
            }
        }
        break;

    case Type::SkyBox:
        renderer.SubmitSkybox(shared_from_this());
        break;

    default:
        break;
    }
}