#pragma once

#include "../Helpers/shaderClass.h"
#include "../Helpers/camera.h"
#include "../../includes/appState.h"
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
class PointLightScene : public Scene
{
public:
    PointLightScene(AppState& appState);
    ~PointLightScene();

    /// Initialize shaders, VAOs, VBOs, and textures
    void init() override;

    /// Update per-frame logic (e.g., light movement, animations)
    void update() override;

    /// Render all objects in the scene
    void render() override;

    /// Scene name for debugging / UI purposes
    std::string name() const override { return "Point Light Demo Scene"; }

private:
    AppState& appState;

    Shader* lightingShader;   // shader used to render the textured cube
    Shader* lightCubeShader;  // shader used to render the small light cube

    unsigned int cubeVAO;     // VAO for the cube
    unsigned int lightCubeVAO;// VAO for the light cube
    unsigned int diffuseMap;  // diffuse texture
    unsigned int specularMap; // specular texture

    // Fixed positions of cubes in the scene
    glm::vec3 cubePositions[10];

    // Animation control for cube rotations
    float rotationAngle;
    float rotationSpeed;

    /// Utility: loads a texture from file and returns the OpenGL texture ID
    unsigned int loadTexture(const char* path);
};