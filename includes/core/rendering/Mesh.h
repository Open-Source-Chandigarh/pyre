#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <cstddef>
#include <string>
#include <vector>
#include <memory>
#include <iostream>
#include "helpers/Shader.h"

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
    TEX_CUBEMAP,
    Other
};

enum class CullMode
{
    Back,
    Front,
    None
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
struct Material {
    std::unordered_map<std::string, std::shared_ptr<Texture>> textures;
    std::unordered_map<std::string, float> floats;
    std::unordered_map<std::string, glm::vec3> vec3s;
    glm::vec3 defaultDiffuseColor = glm::vec3(0.8f);
    float defaultShininess = 32.0f;
    bool outlineEnabled = false;
    glm::vec3 outlineColor = glm::vec3(1.0f);
    bool isTransparent = false;
    CullMode cullMode = CullMode::Back;

    void ApplyToShader(Shader& shader) const
    {
        shader.use();

        int texUnit = 0;

        // --------------------------------------------------------------------
        // STEP 1: Initialize all present flags to 0 IF they actually exist.
        // (Do NOT set them to 0 if shader doesn't declare them)
        // --------------------------------------------------------------------
        const char* presentUniforms[] = {
            "material_diffuse_present",
            "material_specular_present",
            "material_shininess_present"
        };

        for (auto& p : presentUniforms)
            if (shader.hasUniform(p))
                shader.setInt(p, 0);   // default OFF

        // --------------------------------------------------------------------
        // STEP 2: Bind textures & set *_present = 1 for those actually bound.
        // --------------------------------------------------------------------
        for (auto& kv : textures)
        {
            const std::string& name = kv.first;
            const auto& tex = kv.second;

            if (!tex) continue;
            if (!shader.hasUniform(name)) continue;  // shader doesn't want this texture

            GLenum target = (tex->type == TextureType::TEX_CUBEMAP)
                ? GL_TEXTURE_CUBE_MAP
                : GL_TEXTURE_2D;

            glActiveTexture(GL_TEXTURE0 + texUnit);
            glBindTexture(target, tex->ID);

            shader.setInt(name, texUnit);

            // Also set presence flag
            const std::string presentName = name + "_present";
            if (shader.hasUniform(presentName))
                shader.setInt(presentName, 1);

            texUnit++;
        }


        // --------------------------------------------------------------------
        // STEP 3: Push float & vec3 uniforms
        // --------------------------------------------------------------------
        for (auto& kv : floats)
            if (shader.hasUniform(kv.first))
                shader.setFloat(kv.first, kv.second);

        for (auto& kv : vec3s)
            if (shader.hasUniform(kv.first))
                shader.setVec3(kv.first, kv.second);


        // --------------------------------------------------------------------
        // STEP 4: Fallback values if not overridden by textures
        // --------------------------------------------------------------------
        if (shader.hasUniform("material_diffuseColor"))
        {
            if (vec3s.count("material_diffuseColor"))
                shader.setVec3("material_diffuseColor", vec3s.at("material_diffuseColor"));
            else
                shader.setVec3("material_diffuseColor", defaultDiffuseColor);
        }

        if (shader.hasUniform("material_specularColor"))
        {
            if (vec3s.count("material_specularColor"))
                shader.setVec3("material_specularColor", vec3s.at("material_specularColor"));
            else
                shader.setVec3("material_specularColor", glm::vec3(1.0f)); // good default
        }

        if (shader.hasUniform("material_shininess"))
        {
            if (floats.count("material_shininess"))
                shader.setFloat("material_shininess", floats.at("material_shininess"));
            else
                shader.setFloat("material_shininess", defaultShininess);
        }

        glActiveTexture(GL_TEXTURE0);
    }

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