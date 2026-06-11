#pragma once
#include "core/rendering/Mesh.h"
#include "core/rendering/Texture.h"
#include "helpers/Shader.h"
#include <glad/glad.h>
#include <map>
#include <memory>
#include <string>

class ResourceManager
{
  public:
    // Shaders
    static std::shared_ptr<Shader> LoadShader(const std::string &name, const std::string &vsPath,
                                              const std::string &fsPath, const std::string &gsPath = "");
    static std::shared_ptr<Shader> GetShader(const std::string &name);

    // Textures
    static std::shared_ptr<Texture> LoadTexture(const std::string &path, TextureType type);
    static std::shared_ptr<Texture> GetTexture(const std::string &path);
    static std::shared_ptr<Texture> GetFallbackTexture();

    static std::shared_ptr<Texture> LoadCubeMap(std::vector<std::string> faces, std::string name = "");
    static std::shared_ptr<Texture> LoadHDRTexture(const std::string &path);

    // Cleanup GPU resources
    static void Clear();

  private:
    static std::map<std::string, std::shared_ptr<Shader>> shaders;
    static std::map<std::string, std::shared_ptr<Texture>> textures;

    static std::string combinePaths(std::vector<std::string> paths);
};