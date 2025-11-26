#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <cstddef>
#include <string>
#include <vector>
#include <memory>
#include <iostream>
#include "helpers/Shader.h"
#include "core/rendering/Material.h"

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
    Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices);
    Mesh() = default;

    // Creates a mesh from interleaved float data (pos(3), norm(3), uv(2))
    static Mesh CreateFromData(const float* vertices, std::size_t bytes, 
        int vertexCount);

    static Mesh CreateFromIndexedData(const float* vertices, std::size_t vBytes,
        const unsigned int* indices, std::size_t iBytes, int iCount);

    static Mesh CreatePositionsOnly(const float* vertices, 
        std::size_t bytes, int vertexCount);


    void Draw(Shader& shader, Material& material) const;

    // Draw raw geometry (assumes caller set shader and uniforms). Useful for outline pass.
    void DrawSimple() const;

    // Destroy GPU objects
    void Destroy();

    unsigned int VAO = 0;
    unsigned int VBO = 0;
    unsigned int EBO = 0;
    int vertexCount = 0;
    int indexCount = 0;

private:
    void setupMesh();
};