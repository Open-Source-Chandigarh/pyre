#pragma once
#include <glm/glm.hpp>
#include <memory>
#include "helpers/shaderClass.h"
#include "core/rendering/Mesh.h"

class Renderer 
{
public:
    void BeginScene(const glm::mat4& view, const glm::mat4& projection, 
        const glm::vec3& viewPos);
    void Submit(const glm::mat4& model,
        const Mesh& mesh,
        const std::shared_ptr<Shader>& shader,
        unsigned int diffuse = 0,
        unsigned int specular = 0,
        float shininess = 32.0f);
    void EndScene();

private:
    glm::mat4 viewMatrix;
    glm::mat4 projMatrix;
    glm::vec3 viewPosition;
};
