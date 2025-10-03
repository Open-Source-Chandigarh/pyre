#pragma once

#include "../Helpers/shaderClass.h"
#include "../Helpers/camera.h"
#include "scene.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <string>

/**
 * @brief A scene that renders a cube with diffuse + specular maps,
 *        and a light source represented as a small cube.
 */
class DiffuseAndSpecularMapScene : public Scene
{
public:
    DiffuseAndSpecularMapScene();
    ~DiffuseAndSpecularMapScene();

    /// Initialize shaders, VAOs, VBOs, and textures
    void init() override;

    /// Update per-frame logic (e.g., light movement, animations)
    void update(float deltaTime) override;

    /// Render all objects in the scene
    void render() override;

    /// Scene name for debugging / UI purposes
    std::string name() const override { return "Diffuse and Specular Map Scene"; }

private:
    Shader* lightingShader;   // shader used to render the textured cube
    Shader* lightCubeShader;  // shader used to render the small light cube

    unsigned int cubeVAO;     // VAO for the cube
    unsigned int lightCubeVAO;// VAO for the light cube
    unsigned int diffuseMap;  // diffuse texture
    unsigned int specularMap; // specular texture

    /// Utility: loads a texture from file and returns the OpenGL texture ID
    unsigned int loadTexture(const char* path);
};
