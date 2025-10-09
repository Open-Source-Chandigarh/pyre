#include <glad/glad.h>
#include "core/rendering/Renderer.h"

void Renderer::BeginScene(const glm::mat4& view, const glm::mat4& projection,
    const glm::vec3& viewPos) 
{
    viewMatrix = view;
    projMatrix = projection;
    viewPosition = viewPos;
}

void Renderer::Submit(const glm::mat4& model, const Mesh& mesh,
    const std::shared_ptr<Shader>& shader,
    unsigned int diffuse, unsigned int specular, float shininess)
{
    if (!shader) return;
    shader->use();
    shader->setMat4("model", model);
    shader->setMat4("view", viewMatrix);
    shader->setMat4("projection", projMatrix);
    shader->setVec3("viewPos", viewPosition);

    // Material uniforms convention
    shader->setInt("material.diffuse", 0);
    shader->setInt("material.specular", 1);
    shader->setFloat("material.shininess", shininess);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, diffuse);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, specular);

    glBindVertexArray(mesh.VAO);
    glDrawArrays(GL_TRIANGLES, 0, mesh.vertexCount);
    glBindVertexArray(0);
}

void Renderer::EndScene() 
{
    // Currently nothing to flush; place for batching in future
}