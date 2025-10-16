#pragma once
#include <string>
#include <map>
#include <memory>
#include <glad/glad.h>
#include "helpers/shaderClass.h"
#include "core/rendering/Mesh.h"

class ResourceManager
{
public:

    // Shaders
    static std::shared_ptr<Shader> LoadShader(const std::string& name,
        const std::string& vsPath,
        const std::string& fsPath);
    static std::shared_ptr<Shader> GetShader(const std::string& name);

    // Textures
    static std::shared_ptr<Texture> LoadTexture(const std::string& path, TextureType type);
    static std::shared_ptr<Texture> GetTexture(const std::string& path);

    // Cleanup GPU resources
    static void Clear();

private:

    static std::map<std::string, std::shared_ptr<Shader>> shaders;
    static std::map<std::string, std::shared_ptr<Texture>> textures;

};