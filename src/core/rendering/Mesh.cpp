#define _USE_MATH_DEFINES
#include "core/rendering/Mesh.h"
#include "core/rendering/Material.h"
#include <array>
#include <cmath>

Mesh::Mesh(Mesh &&other) noexcept
    : vertices(std::move(other.vertices)), indices(std::move(other.indices)),
      localMaterial(std::move(other.localMaterial)), VAO(other.VAO), VBO(other.VBO), EBO(other.EBO),
      instanceVBO(other.instanceVBO), vertexCount(other.vertexCount), indexCount(other.indexCount)
{
    // Reset other to zero so its destructor doesn't kill our resources
    other.VAO = 0;
    other.VBO = 0;
    other.EBO = 0;
    other.instanceVBO = 0;
    other.vertexCount = 0;
    other.indexCount = 0;
}

Mesh &Mesh::operator=(Mesh &&other) noexcept
{
    if (this != &other)
    {
        Destroy();

        // Steal resources
        VAO = other.VAO;
        VBO = other.VBO;
        EBO = other.EBO;
        instanceVBO = other.instanceVBO;
        vertexCount = other.vertexCount;
        indexCount = other.indexCount;
        vertices = std::move(other.vertices);
        indices = std::move(other.indices);
        localMaterial = std::move(other.localMaterial);
        other.VAO = 0;
        other.VBO = 0;
        other.EBO = 0;
        other.instanceVBO = 0;
        other.vertexCount = 0;
        other.indexCount = 0;
    }
    return *this;
}

Mesh::Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, std::shared_ptr<Material> mat)
    : vertices(vertices), indices(indices), localMaterial(mat)
{
    setupMesh();
}

Mesh::~Mesh()
{
    Destroy();
}

void Mesh::setupMesh()
{
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

    // vertex positions
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *) 0);
    // vertex normals
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *) offsetof(Vertex, Normal));
    // vertex texture coords
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *) offsetof(Vertex, TexCoords));
    // vertex tangent
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *) offsetof(Vertex, Tangent));
    glBindVertexArray(0);
}

void Mesh::SetupInstancing(const std::vector<glm::mat4> &models)
{
    glBindVertexArray(VAO);

    // Create Buffer
    glGenBuffers(1, &instanceVBO);
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    glBufferData(GL_ARRAY_BUFFER, models.size() * sizeof(glm::mat4), models.data(), GL_STATIC_DRAW);

    // Setup Attributes (mat4 requires 4 vec4 slots)
    std::size_t vec4Size = sizeof(glm::vec4);

    for (int i = 0; i < 4; i++)
    {
        glEnableVertexAttribArray(4 + i);
        glVertexAttribPointer(4 + i, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void *) (i * vec4Size));
        // Tell OpenGL this attribute changes per INSTANCE, not per vertex
        glVertexAttribDivisor(4 + i, 1);
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

// just bind and issue draw call (no texture binding/no shader use)
void Mesh::DrawSimple() const
{
    glBindVertexArray(VAO);
    if (!indices.empty())
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, 0);
    else if (indexCount > 0)
        glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
    else
        glDrawArrays(GL_TRIANGLES, 0, vertexCount);
    glBindVertexArray(0);
}

void Mesh::Draw(Shader &shader) const
{
    shader.use();
    shader.setBool("isInstanced", false);

    glBindVertexArray(VAO);
    if (!indices.empty())
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, 0);
    else if (indexCount > 0)
        glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
    else
        glDrawArrays(GL_TRIANGLES, 0, vertexCount);

    glBindVertexArray(0);
    glActiveTexture(GL_TEXTURE0);
}

void Mesh::DrawInstanced(Shader &shader, int count) const
{
    shader.use();
    shader.setBool("isInstanced", true); // Trigger the shader switch

    glBindVertexArray(VAO);
    if (!indices.empty())
    {
        glDrawElementsInstanced(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, 0, count);
    }
    else
    {
        glDrawArraysInstanced(GL_TRIANGLES, 0, vertexCount, count);
    }

    glBindVertexArray(0);

    shader.setBool("isInstanced", false); // Reset state
    glActiveTexture(GL_TEXTURE0);
}

void Mesh::Destroy()
{
    if (instanceVBO)
        glDeleteBuffers(1, &instanceVBO);
    if (EBO)
        glDeleteBuffers(1, &EBO);
    if (VBO)
        glDeleteBuffers(1, &VBO);
    if (VAO)
        glDeleteVertexArrays(1, &VAO);
    // Reset to 0
    VAO = VBO = EBO = instanceVBO = 0;
}

Mesh Mesh::CreateFromData(const float *vertices, std::size_t bytes, int vCount)
{
    Mesh m;
    glGenVertexArrays(1, &m.VAO);
    glGenBuffers(1, &m.VBO);

    glBindVertexArray(m.VAO);
    glBindBuffer(GL_ARRAY_BUFFER, m.VBO);
    glBufferData(GL_ARRAY_BUFFER, bytes, vertices, GL_STATIC_DRAW);

    // Layout: pos(0) normal(1) tex(2), stride = 8 floats
    constexpr GLsizei stride = 8 * sizeof(float);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void *) 0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void *) (3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void *) (6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
    m.vertexCount = vCount;
    return m;
}

Mesh Mesh::CreatePositionsOnly(const float *vertices, std::size_t bytes, int vertexCount)
{
    Mesh m;
    glGenVertexArrays(1, &m.VAO);
    glGenBuffers(1, &m.VBO);

    glBindVertexArray(m.VAO);
    glBindBuffer(GL_ARRAY_BUFFER, m.VBO);
    glBufferData(GL_ARRAY_BUFFER, bytes, vertices, GL_STATIC_DRAW);

    // Layout: only position (location = 0)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *) 0);

    glBindVertexArray(0);
    m.vertexCount = vertexCount;
    return m;
}

Mesh Mesh::CreateFromIndexedData(const float *vertices, std::size_t vBytes, const unsigned int *indices,
                                 std::size_t iBytes, int iCount)
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
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void *) 0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void *) (3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void *) (6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);

    m.indexCount = iCount;
    return m;
}