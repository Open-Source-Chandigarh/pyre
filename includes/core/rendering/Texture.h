#pragma once
#include <glad/glad.h>
#include <string>
#include <vector>
#include "helpers/Shader.h"

// Texture type
enum class TextureType
{
    TEX_DIFFUSE,
    TEX_SPECULAR,
    TEX_CUBEMAP,
    Other
};

// RAII wrapper for an OpenGL texture.
// non-copyable (to avoid double-delete)
// movable (transfers ownership)
// destructor deletes GL handle (must run with GL context current)
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
            // glDeleteTextures must be called with a valid GL context.
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
    Texture& operator=(Texture&& other) noexcept 
    {
        if (this != &other) {
            if (ID) glDeleteTextures(1, &ID);
            steal(other);
        }
        return *this;
    }

private:
    void steal(Texture& other) noexcept 
    {
        ID = other.ID;
        type = other.type;
        path = std::move(other.path);
        width = other.width;
        height = other.height;
        channels = other.channels;
        other.ID = 0;
    }
};