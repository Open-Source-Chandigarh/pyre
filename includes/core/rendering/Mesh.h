#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <cstddef>
#include <string>
#include <vector>
#include "helpers/shaderClass.h"

struct Vertex
{
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoords;
};

struct Texture
{
    unsigned int ID;
    std::string type;
    std::string path;
};


class Mesh
{
public:

    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture> textures;
    Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices,
        std::vector<Texture> textures);
    Mesh() = default;

    // Creates a mesh from interleaved float data (pos(3), norm(3), uv(2))
    static Mesh CreateFromData(const float* vertices, std::size_t bytes, int vertexCount);

    static Mesh CreateFromIndexedData(const float* vertices, std::size_t vBytes,
        const unsigned int* indices, std::size_t iBytes, int iCount);

    void Draw(Shader& shader);

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