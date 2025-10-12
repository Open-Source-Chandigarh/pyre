#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <cstddef>
#include <string>
#include <vector>
#include <memory>
#include "helpers/shaderClass.h"

// ----------------------------------------------------------------------------
// POD vertex
struct Vertex
{
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoords;
};

// ----------------------------------------------------------------------------
// Texture type
enum class TextureType
{
    TEX_DIFFUSE,
    TEX_SPECULAR,
    Other
};

// RAII wrapper for an OpenGL texture.
// - non-copyable (to avoid double-delete)
// - movable (transfers ownership)
// - destructor deletes GL handle (must run with GL context current)
struct Texture
{
    unsigned int ID = 0;
    TextureType type = TextureType::Other;
    std::string path;
    int width = 0;
    int height = 0;
    int channels = 0;

    Texture() = default;

    // ensure GL resource is freed when object is destroyed
    ~Texture() {
        if (ID != 0) {
            // IMPORTANT: glDeleteTextures must be called with a valid GL context.
            // Make sure ResourceManager::Clear() (or similar) is called before the GL context is destroyed.
            glDeleteTextures(1, &ID);
            ID = 0;
        }
    }

    // non-copyable
    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

    // movable
    Texture(Texture&& other) noexcept {
        steal(other);
    }
    Texture& operator=(Texture&& other) noexcept {
        if (this != &other) {
            if (ID) glDeleteTextures(1, &ID);
            steal(other);
        }
        return *this;
    }

private:
    void steal(Texture& other) noexcept {
        ID = other.ID;
        type = other.type;
        path = std::move(other.path);
        width = other.width;
        height = other.height;
        channels = other.channels;
        other.ID = 0;
    }
};

// ----------------------------------------------------------------------------
// Material : describes surface appearance and references textures (shared)
// - store shared_ptr<Texture> so multiple materials/entities can share the same Texture
struct Material
{
    std::vector<std::shared_ptr<Texture>> textures; // texture resources, optional
    glm::vec3 diffuseColor = glm::vec3(0.8f, 0.8f, 0.8f);
    glm::vec3 specularColor = glm::vec3(1.0f, 1.0f, 1.0f);
    float shininess = 32.0f;
    bool useDiffuseMap = false;
    bool useSpecularMap = false;

    Material() = default;

    // convenience: find first texture of given type
    std::shared_ptr<Texture> GetTexture(TextureType t) const {
        for (auto& tex : textures) {
            if (tex && tex->type == t) return tex;
        }
        return nullptr;
    }

    // helper booleans
    bool HasDiffuseTexture() const { return useDiffuseMap && (GetTexture(TextureType::TEX_DIFFUSE) != nullptr); }
    bool HasSpecularTexture() const { return useSpecularMap && (GetTexture(TextureType::TEX_SPECULAR) != nullptr); }
};


class Mesh
{
public:

    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices);
    Mesh() = default;

    // Creates a mesh from interleaved float data (pos(3), norm(3), uv(2))
    static Mesh CreateFromData(const float* vertices, std::size_t bytes, int vertexCount);

    static Mesh CreateFromIndexedData(const float* vertices, std::size_t vBytes,
        const unsigned int* indices, std::size_t iBytes, int iCount);

    void Draw(Shader& shader, Material& material) const;

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