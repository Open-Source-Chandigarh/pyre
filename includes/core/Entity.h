#pragma once
#include "core/rendering/Model.h"
#include "core/rendering/Renderer.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <memory>
#include <string>

class Mesh;
class Shader;
struct Material;

struct Transform
{
    glm::vec3 position{0.0f};
    glm::vec3 scale{1.0f};
    glm::vec3 rotation{0.0f}; // Euler angles in degrees

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

// Unified Render Component
struct RenderComponent
{
    // the list of things to draw (backpack parts, or a single Cube)
    std::vector<ModelNode> nodes;

    // low poly model to render shadow maps (this increases performance with no major drawbacks)
    std::shared_ptr<Model> shadowProxyModel = nullptr;

    // Material Override
    // Properties here (e.g., "OutlineEnabled=true") will be mixed with the Mesh's Base Material.
    // If this is null, the Mesh Base Material is used as is or if base material is null this is used instead.
    std::shared_ptr<Material> materialOverride;

    std::shared_ptr<Shader> shader;
    int instanceCount = 1;
};

struct SkyboxComponent
{
    Mesh *mesh = nullptr;
    std::shared_ptr<Material> material; // Should contain the Cubemap texture
    std::shared_ptr<Shader> shader;
};

struct Entity
{
    std::string name;
    Transform transform;

    // Component Slots
    std::shared_ptr<RenderComponent> renderComp;
    std::shared_ptr<SkyboxComponent> skyboxComp;

    static std::shared_ptr<Entity> Create(const std::string &name = "Entity")
    {
        auto e = std::make_shared<Entity>();
        e->name = name;
        return e;
    }

    std::shared_ptr<Material> GetUniqueMaterial()
    {
        if (!renderComp)
            return nullptr;

        if (!renderComp->materialOverride)
        {
            // Clone the first mesh's material as a base if it exists
            if (!renderComp->nodes.empty() && renderComp->nodes[0].mesh && renderComp->nodes[0].mesh->localMaterial)
            {
                renderComp->materialOverride = std::make_shared<Material>(*renderComp->nodes[0].mesh->localMaterial);
            }
            else
            {
                renderComp->materialOverride = std::make_shared<Material>();
            }
        }
        else if (renderComp->materialOverride.use_count() > 1)
        {
            auto newMat = std::make_shared<Material>(*renderComp->materialOverride);
            renderComp->materialOverride = newMat;
        }

        // pull base mesh textures into override so they show up in the editor
        if (!renderComp->nodes.empty() && renderComp->nodes[0].mesh && renderComp->nodes[0].mesh->localMaterial)
        {
            auto baseMat = renderComp->nodes[0].mesh->localMaterial;
            for (const auto &[key, tex] : baseMat->textures)
            {
                if (renderComp->materialOverride->textures.find(key) == renderComp->materialOverride->textures.end())
                {
                    renderComp->materialOverride->textures[key] = tex;
                }
            }
        }

        return renderComp->materialOverride;
    }

    // Helper Methods to Add Components

    void AddMesh(std::shared_ptr<Mesh> mesh, std::shared_ptr<Material> mat, std::shared_ptr<Shader> shader,
                 int instanceCount = 1)
    {
        renderComp = std::make_shared<RenderComponent>();

        ModelNode node;
        node.mesh = mesh;
        node.localTransform = glm::mat4(1.0f);

        renderComp->nodes.push_back(node);
        renderComp->shader = shader;
        renderComp->materialOverride = mat;
        renderComp->instanceCount = instanceCount;
    }

    void AddModel(Model *model, std::shared_ptr<Shader> shader, std::shared_ptr<Material> overrideMat = nullptr,
                  int instanceCount = 1)
    {
        renderComp = std::make_shared<RenderComponent>();
        renderComp->nodes = model->nodes; // Copy the list of nodes
        renderComp->shader = shader;
        renderComp->materialOverride = overrideMat;
        renderComp->instanceCount = instanceCount;
    }

    void AddSkybox(Mesh *mesh, std::shared_ptr<Material> mat, std::shared_ptr<Shader> shader)
    {
        skyboxComp = std::make_shared<SkyboxComponent>();
        skyboxComp->mesh = mesh;
        skyboxComp->material = mat;
        skyboxComp->shader = shader;
    }
};