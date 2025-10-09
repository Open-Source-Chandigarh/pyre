#pragma once
#include <string>
#include <map>
#include <memory>
#include <glad/glad.h>
#include "helpers/shaderClass.h"

class ResourceManager
{
public:

    // Shaders
    static std::shared_ptr<Shader> LoadShader(const std::string& name,
        const std::string& vsPath,
        const std::string& fsPath);
    static std::shared_ptr<Shader> GetShader(const std::string& name);

    // Textures
    static unsigned int LoadTexture(const std::string& name, const std::string& path);
    static unsigned int GetTexture(const std::string& name);

    // Cleanup GPU resources
    static void Clear();

private:

    static std::map<std::string, std::shared_ptr<Shader>> shaders;
    static std::map<std::string, unsigned int> textures;

};