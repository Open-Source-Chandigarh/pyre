#define _USE_MATH_DEFINES
#include <cmath>
#include <array>
#include "core/rendering/Mesh.h"

Mesh::Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, Material& material) : 
    vertices(vertices), indices(indices), material(material)
{
    setupMesh();
}

void Mesh::setupMesh()
{
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex),
        &vertices[0], GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int),
        &indices[0], GL_STATIC_DRAW);

    // vertex positions
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
        (void*)0);
    // vertex normals
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
        (void*)offsetof(Vertex, Normal));
    // vertex texture coords
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
        (void*)offsetof(Vertex, TexCoords));
    glBindVertexArray(0);
}

void Mesh::Draw(Shader& shader) const
{
    shader.use();

    // ---------------------------
    // Handle texture binding
    // ---------------------------
    unsigned int diffuseID = 0;
    unsigned int specularID = 0;

    for (const auto& tex : material.textures) {
        if (tex.type == "texture_diffuse" && diffuseID == 0)
            diffuseID = tex.ID;
        else if (tex.type == "texture_specular" && specularID == 0)
            specularID = tex.ID;
    }

    // Bind textures (if available)
    if (material.useDiffuseMap && diffuseID != 0) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, diffuseID);
        shader.setInt("material.diffuse", 0);
    }
    else {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0); // no texture
    }

    if (material.useSpecularMap && specularID != 0) {
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, specularID);
        shader.setInt("material.specular", 1);
    }
    else {
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    // ---------------------------
    // Send all material uniforms
    // ---------------------------
    shader.setBool("material.useDiffuseMap", material.useDiffuseMap);
    shader.setBool("material.useSpecularMap", material.useSpecularMap);
    shader.setVec3("material.diffuseColor", material.diffuseColor);
    shader.setVec3("material.specularColor", material.specularColor);
    shader.setFloat("material.shininess", material.shininess);

    // ---------------------------
    // Draw
    // ---------------------------
    glBindVertexArray(VAO);
    if (!indices.empty())
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, 0);
    else if (indexCount > 0)
        glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
    else
        glDrawArrays(GL_TRIANGLES, 0, vertexCount);
    glBindVertexArray(0);

    // ---------------------------
    // Reset state
    // ---------------------------
    glActiveTexture(GL_TEXTURE0);
}

Mesh Mesh::CreateFromData(const float* vertices, std::size_t bytes, int vCount) 
{
    Mesh m;
    glGenVertexArrays(1, &m.VAO);
    glGenBuffers(1, &m.VBO);

    glBindVertexArray(m.VAO);
    glBindBuffer(GL_ARRAY_BUFFER, m.VBO);
    glBufferData(GL_ARRAY_BUFFER, bytes, vertices, GL_STATIC_DRAW);

    // Layout: pos(0) normal(1) tex(2), stride = 8 floats
    constexpr GLsizei stride = 8 * sizeof(float);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
    m.vertexCount = vCount;
    return m;
}

Mesh Mesh::CreateFromIndexedData(const float* vertices, std::size_t vBytes,
    const unsigned int* indices, std::size_t iBytes, int iCount)
{
    Mesh m;
    glGenVertexArrays(1, &m.VAO);
    glGenBuffers(1, &m.VBO);
    glGenBuffers(1, &m.EBO);

    glBindVertexArray(m.VAO);

    glBindBuffer(GL_ARRAY_BUFFER, m.VBO);
    glBufferData(GL_ARRAY_BUFFER, vBytes, vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, iBytes, indices, GL_STATIC_DRAW);

    constexpr GLsizei stride = 8 * sizeof(float);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);

    m.indexCount = iCount;
    return m;
}

void Mesh::Destroy() 
{
    if (VBO) { glDeleteBuffers(1, &VBO); VBO = 0; }
    if (VAO) { glDeleteVertexArrays(1, &VAO); VAO = 0; }
    if (EBO) { glDeleteBuffers(1, &EBO); EBO = 0; }
}