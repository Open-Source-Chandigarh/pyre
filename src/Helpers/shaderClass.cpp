#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <unordered_set>
#include <filesystem>
#include <regex>
#include "helpers/ShaderClass.h"

Shader::Shader(const char* vertexPath, const char* fragmentPath) {
    std::string vertexCode = preprocessShaderIncludes(vertexPath);
    std::string fragmentCode = preprocessShaderIncludes(fragmentPath);
    this->vertexPath = vertexPath;
    const char* vShaderCode = vertexCode.c_str();
    const char* fShaderCode = fragmentCode.c_str();

    unsigned int vertex = compileShader(GL_VERTEX_SHADER, vShaderCode);
    unsigned int fragment = compileShader(GL_FRAGMENT_SHADER, fShaderCode);

    ID = glCreateProgram();
    glAttachShader(ID, vertex);
    glAttachShader(ID, fragment);
    glLinkProgram(ID);

    checkCompileErrors(ID, "PROGRAM");

    bindUBO("GlobalLights", 0);

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

void Shader::bindUBO(const std::string& blockName, GLuint bindingPoint)
{
    GLuint index = glGetUniformBlockIndex(ID, blockName.c_str());
    if (index == GL_INVALID_INDEX) {
        std::cerr << "Warning: UBO block '" << blockName << "' not found in shader" << ": " << vertexPath << "\n";
        return;
    }
    glUniformBlockBinding(ID, index, bindingPoint);
}

void Shader::setBool(const std::string& name, bool value) const { 
    if (!hasUniform(name)) return;
    glUniform1i(getUniformLocation(name), (int)value); 
}
void Shader::setInt(const std::string& name, int value) const { 
    if (!hasUniform(name)) return;
    glUniform1i(getUniformLocation(name), value); 
}
void Shader::setFloat(const std::string& name, float value) const {
    if (!hasUniform(name)) return;
    glUniform1f(getUniformLocation(name), value); 
}
void Shader::setMat4(const std::string& name, const glm::mat4& value) const { 
    if (!hasUniform(name)) return;
    glUniformMatrix4fv(getUniformLocation(name), 1, GL_FALSE, glm::value_ptr(value)); 
}
void Shader::setVec3(const std::string& name, const glm::vec3& value) const { 
    if (!hasUniform(name)) return;
    glUniform3fv(getUniformLocation(name), 1, glm::value_ptr(value)); 
}
void Shader::setVec3(const std::string& name, float x, float y, float z) const { 
    if (!hasUniform(name)) return;
    setVec3(name, glm::vec3(x, y, z)); 
}

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

std::string Shader::loadFileToString(const std::string& path){
    std::ifstream file(path);

    if (!file.is_open()) {
        std::cerr << "Shader: failed to open " << path << "\n";
        return "";
    }

    std::stringstream ss; 
    ss << file.rdbuf();
    return ss.str();
}

std::string Shader::resolveIncludePath(const std::string& baseFile, const std::string& includePath){
    std::filesystem::path base(baseFile);
    auto parent = base.parent_path();
    std::filesystem::path resolved = parent / includePath;
    return resolved.lexically_normal().string();
}

std::string Shader::preprocessInternal(const std::string& filename, std::unordered_set<std::string>& visited){
    if (visited.count(filename)) return "";

    visited.insert(filename);
    std::string src = loadFileToString(filename);
    if (src.empty()) return "";

    std::stringstream out;
    std::istringstream in(src);
    std::string line;
    std::regex includeRegex(R"(^\s*#\s*include\s*\"(.+)\"\s*)");

    while (std::getline(in, line)) {
        std::smatch m;
        if (std::regex_search(line, m, includeRegex)) {
            std::string inc = m[1].str();
            std::string resolved = resolveIncludePath(filename, inc);
            out << "// Begin include: " << inc << "\n";
            out << preprocessInternal(resolved, visited);
            out << "// End include: " << inc << "\n";
        }
        else out << line << "\n";
    }
    return out.str();
}

std::string Shader::preprocessShaderIncludes(const std::string& filename){
    std::unordered_set<std::string> visited;
    return preprocessInternal(filename, visited);
}

bool Shader::hasUniform(const std::string& name) const {
    return getUniformLocation(name.c_str()) != -1;
}