#include "helpers/shaderClass.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept>

Shader::Shader(const char* vertexPath, const char* fragmentPath) {
    std::string vertexCode, fragmentCode;
    try {
        std::ifstream vFile(vertexPath);
        std::ifstream fFile(fragmentPath);
        if (!vFile.is_open() || !fFile.is_open())
            throw std::runtime_error("Cannot open shader file");

        std::stringstream vStream, fStream;
        vStream << vFile.rdbuf();
        fStream << fFile.rdbuf();
        vertexCode = vStream.str();
        fragmentCode = fStream.str();
    }
    catch (const std::exception& e) {
        std::cerr << "ERROR::SHADER::FILE_NOT_READ\n" << e.what() << std::endl;
    }

    const char* vShaderCode = vertexCode.c_str();
    const char* fShaderCode = fragmentCode.c_str();

    unsigned int vertex = compileShader(GL_VERTEX_SHADER, vShaderCode);
    unsigned int fragment = compileShader(GL_FRAGMENT_SHADER, fShaderCode);

    ID = glCreateProgram();
    glAttachShader(ID, vertex);
    glAttachShader(ID, fragment);
    glLinkProgram(ID);
    checkCompileErrors(ID, "PROGRAM");

    glDeleteShader(vertex);
    glDeleteShader(fragment);
}

Shader::~Shader() {
    if (ID) glDeleteProgram(ID);
}

Shader::Shader(Shader&& other) noexcept : ID(other.ID) {
    other.ID = 0;
}

Shader& Shader::operator=(Shader&& other) noexcept {
    if (this != &other) {
        if (ID) glDeleteProgram(ID);
        ID = other.ID;
        other.ID = 0;
    }
    return *this;
}

void Shader::use() const { glUseProgram(ID); }

void Shader::setBool(const std::string& name, bool value) const { glUniform1i(getUniformLocation(name), (int)value); }
void Shader::setInt(const std::string& name, int value) const { glUniform1i(getUniformLocation(name), value); }
void Shader::setFloat(const std::string& name, float value) const { glUniform1f(getUniformLocation(name), value); }
void Shader::setMat4(const std::string& name, const glm::mat4& value) const { glUniformMatrix4fv(getUniformLocation(name), 1, GL_FALSE, glm::value_ptr(value)); }
void Shader::setVec3(const std::string& name, const glm::vec3& value) const { glUniform3fv(getUniformLocation(name), 1, glm::value_ptr(value)); }
void Shader::setVec3(const std::string& name, float x, float y, float z) const { setVec3(name, glm::vec3(x, y, z)); }

int Shader::getUniformLocation(const std::string& name) const {
    if (uniformCache.find(name) != uniformCache.end()) return uniformCache[name];
    int location = glGetUniformLocation(ID, name.c_str());
    if (location == -1) std::cerr << "WARNING::SHADER::UNIFORM '" << name << "' NOT FOUND\n";
    uniformCache[name] = location;
    return location;
}

unsigned int Shader::compileShader(unsigned int type, const char* code) const {
    unsigned int shader = glCreateShader(type);
    glShaderSource(shader, 1, &code, nullptr);
    glCompileShader(shader);
    checkCompileErrors(shader, type == GL_VERTEX_SHADER ? "VERTEX" : "FRAGMENT");
    return shader;
}

void Shader::checkCompileErrors(unsigned int shader, const std::string& type) const {
    int success;
    char infoLog[1024];
    if (type != "PROGRAM") {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(shader, 1024, nullptr, infoLog);
            std::cerr << "ERROR::SHADER::" << type << "::COMPILATION_FAILED\n" << infoLog << std::endl;
        }
    }
    else {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(shader, 1024, nullptr, infoLog);
            std::cerr << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
        }
    }
}