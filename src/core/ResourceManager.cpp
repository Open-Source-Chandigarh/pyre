#include <iostream>
#include <stb_image.h>
#include "core/ResourceManager.h"

std::map<std::string, std::shared_ptr<Shader>> ResourceManager::shaders;
std::map<std::string, std::shared_ptr<Texture>> ResourceManager::textures;

std::shared_ptr<Shader> ResourceManager::LoadShader(const std::string& name,
    const std::string& vsPath, const std::string& fsPath)
{
    auto it = shaders.find(name);
    if (it != shaders.end()) return it->second;
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

std::shared_ptr<Texture> ResourceManager::LoadTexture(const std::string& path, TextureType type)
{
    if (textures.count(path)) return textures[path];

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
    
    if(format != GL_RGBA)
    { 
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    }
 
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(data);

    std::shared_ptr<Texture> texture = std::make_shared<Texture>();
    texture->ID = tex;
    texture->type = type;
    texture->width = width;
    texture->height = height;
    texture->channels = nrChannels;

    textures[path] = texture;
    return texture;
}

std::shared_ptr<Texture> ResourceManager::LoadCubeMap(
    std::vector<std::string> faces, std::string name)
{
    std::string key = name.empty() ? combinePaths(faces) : name;
    if (textures.count(key)) return textures[key];
    stbi_set_flip_vertically_on_load(false);

    unsigned int tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_CUBE_MAP, tex);

    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++)
    {
        unsigned char* data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        GLenum format = (nrChannels == 1) ? GL_RED : (nrChannels == 3) ? GL_RGB : GL_RGBA;
        if (data)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data
            );
            stbi_image_free(data);
        }
        else
        {
            std::cout << "Cubemap tex failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    std::shared_ptr<Texture> texture = std::make_shared<Texture>();
    texture->ID = tex;
    texture->type = TextureType::TEX_CUBEMAP;
    texture->width = width;
    texture->height = height;
    texture->channels = nrChannels;

    textures[key] = texture;
    return texture;
}

std::shared_ptr<Texture> ResourceManager::GetTexture(const std::string& path)
{
    auto it = textures.find(path);
    if (it == textures.end()) return 0;
    return it->second;
}

std::string ResourceManager::combinePaths(std::vector<std::string> paths)
{
    std::string key = "";
    for (auto&& s : paths)
    {
        key += s + "#"; // # for seperation
    }
    return key;
}

void ResourceManager::Clear() 
{
    textures.clear();
    shaders.clear();
}