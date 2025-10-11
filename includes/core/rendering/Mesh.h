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

struct Material 
{
    std::vector<Texture> textures;
    glm::vec3 diffuseColor = glm::vec3(0.8f, 0.8f, 0.8f);
    glm::vec3 specularColor = glm::vec3(1.0f, 1.0f, 1.0f);
    float shininess = 32.0f;
    bool useDiffuseMap = false;
    bool useSpecularMap = false;
};


class Mesh
{
public:

    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    Material material;
    Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, Material& material);
    Mesh() = default;

    // Creates a mesh from interleaved float data (pos(3), norm(3), uv(2))
    static Mesh CreateFromData(const float* vertices, std::size_t bytes, int vertexCount);

    static Mesh CreateFromIndexedData(const float* vertices, std::size_t vBytes,
        const unsigned int* indices, std::size_t iBytes, int iCount);

    void Draw(Shader& shader) const;
    void SetMaterial(const Material& m) { material = m; }
    const Material& GetMaterial() const { return material; }

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