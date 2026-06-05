#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>
#include <unordered_map>
#include <unordered_set>

class Shader
{
  public:
    unsigned int ID;

    // Constructor
    Shader(const char *vertexPath, const char *fragmentPath, const char *geometryPath = nullptr);
    ~Shader();

    // Delete copy, allow move
    Shader(const Shader &) = delete;
    Shader &operator=(const Shader &) = delete;
    Shader(Shader &&other) noexcept;
    Shader &operator=(Shader &&other) noexcept;

    void use() const;

    // Uniform setters
    void setBool(const std::string &name, bool value) const;
    void setInt(const std::string &name, int value) const;
    void setFloat(const std::string &name, float value) const;
    void setMat4(const std::string &name, const glm::mat4 &value) const;
    void setVec3(const std::string &name, const glm::vec3 &value) const;
    void setVec3(const std::string &name, float x, float y, float z) const;

    void bindUBO(const std::string &blockName, GLuint bindingPoint);

    bool hasUniform(const std::string &name) const;

  private:
    mutable std::unordered_map<std::string, int> uniformCache;

    int getUniformLocation(const std::string &name) const;
    std::string loadFileToString(const std::string &path);
    std::string preprocessShaderIncludes(const std::string &filename);
    std::string preprocessInternal(const std::string &filename, std::unordered_set<std::string> &visited);
    std::string resolveIncludePath(const std::string &baseFile, const std::string &includePath);
    unsigned int compileShader(unsigned int type, const char *code) const;
    void checkCompileErrors(unsigned int shader, const std::string &type) const;
};
