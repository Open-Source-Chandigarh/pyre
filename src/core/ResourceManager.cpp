#include <iostream>
#include <stb_image.h>
#include "core/ResourceManager.h"

std::map<std::string, std::shared_ptr<Shader>> ResourceManager::shaders;
std::map<std::string, unsigned int> ResourceManager::textures;

std::shared_ptr<Shader> ResourceManager::LoadShader(const std::string& name,
    const std::string& vsPath, const std::string& fsPath)
{
    try {
        auto s = std::make_shared<Shader>(vsPath.c_str(), fsPath.c_str());
        shaders[name] = s;
        return s;
    }
    catch (const std::exception& e) {
        std::cerr << "ResourceManager::LoadShader failed: " << e.what() << "\n";
        return nullptr;
    }
}

std::shared_ptr<Shader> ResourceManager::GetShader(const std::string& name) 
{
    auto it = shaders.find(name);
    if (it == shaders.end()) return nullptr;
    return it->second;
}

unsigned int ResourceManager::LoadTexture(const std::string& name, const std::string& path) 
{
    if (textures.count(name)) return textures[name];

    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &nrChannels, 0);
    if (!data) {
        std::cerr << "ResourceManager: Failed to load texture " << path << "\n";
        return 0;
    }

    GLenum format = (nrChannels == 1) ? GL_RED : (nrChannels == 3) ? GL_RGB : GL_RGBA;

    unsigned int tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(data);

    textures[name] = tex;
    return tex;
}

unsigned int ResourceManager::GetTexture(const std::string& name) 
{
    auto it = textures.find(name);
    if (it == textures.end()) return 0;
    return it->second;
}

void ResourceManager::Clear() 
{
    for (auto& p : textures) {
        glDeleteTextures(1, &p.second);
    }
    textures.clear();
    shaders.clear();
}