#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <cstddef>
#include <string>
#include <vector>
#include <memory>
#include <iostream>
#include "helpers/Shader.h"

class Material;

// POD vertex
struct Vertex
{
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoords;
};

class Mesh
{
public:

    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    unsigned int VAO = 0;
    unsigned int VBO = 0;
    unsigned int EBO = 0;
    unsigned int instanceVBO = 0;
    int vertexCount = 0;
    int indexCount = 0;

    Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices);
    Mesh() = default;
    ~Mesh();
    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;
    Mesh(Mesh&& other) noexcept;
    Mesh& operator=(Mesh&& other) noexcept;

    // Factories
    static Mesh CreateFromData(const float* vertices, size_t bytes, int vertexCount);
    static Mesh CreateFromIndexedData(const float* vertices, size_t vBytes, const unsigned int* indices, size_t iBytes, int iCount);
    static Mesh CreatePositionsOnly(const float* vertices, size_t bytes, int vertexCount);

    // Drawing
    void Draw(Shader &shader, const Material &material) const;
    void DrawSimple() const;
    void DrawInstanced(Shader &shader, const Material &material, int count) const;
    void SetupInstancing(const std::vector<glm::mat4> &models);

private:
    void setupMesh();
    void Destroy(); // Internal helper
};