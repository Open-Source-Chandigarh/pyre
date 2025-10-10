#include <glad/glad.h>
#include "core/rendering/Renderer.h"
#include "core/rendering/Model.h"

void Renderer::BeginScene(const glm::mat4& view, const glm::mat4& projection,
    const glm::vec3& viewPos) 
{
    viewMatrix = view;
    projMatrix = projection;
    viewPosition = viewPos;
}

void Renderer::SubmitMesh(const glm::mat4& model, const Mesh& mesh,
    const std::shared_ptr<Shader>& shader,
    unsigned int diffuse, unsigned int specular, float shininess)
{
    if (!shader) return;

    shader->use();
    shader->setMat4("model", model);
    shader->setMat4("view", viewMatrix);
    shader->setMat4("projection", projMatrix);
    shader->setVec3("viewPos", viewPosition);

    // Material uniforms
    shader->setInt("material.diffuse", 0);
    shader->setInt("material.specular", 1);
    shader->setFloat("material.shininess", shininess);

    // Bind textures
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, diffuse);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, specular);

    glBindVertexArray(mesh.VAO);

    if (mesh.EBO != 0 && mesh.indexCount > 0)
    {
        // Indexed mesh
        glDrawElements(GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_INT, nullptr);
    }
    else
    {
        // Non-indexed fallback
        glDrawArrays(GL_TRIANGLES, 0, mesh.vertexCount);
    }

    glBindVertexArray(0);
}

void Renderer::SubmitModel(const glm::mat4& model, Model& modelObj, const std::shared_ptr<Shader>& shader)
{
    if (!shader) return;

    // ensure shader is active and scene uniforms are set
    shader->use();
    shader->setMat4("view", viewMatrix);
    shader->setMat4("projection", projMatrix);
    shader->setVec3("viewPos", viewPosition);

    // set model 
    shader->setMat4("model", model);

    // call the model draw which will iterate meshes
    modelObj.Draw(*shader);
}


void Renderer::EndScene() 
{
    // Currently nothing to flush; place for batching in future
}