#pragma once
#include "Scene.h"
#include "../Helpers/shaderClass.h"
#include "../Helpers/camera.h"
#include <glm/glm.hpp>
#include <vector>

// This class represents a Scene that demonstrates lighting with a single directional light.
// It derives from the base Scene class, so it must implement init(), update(), render(), and name().
class FlashLightScene : public Scene {
public:
    FlashLightScene();
    ~FlashLightScene();

    // Called once when the scene is created (setup VAOs, VBOs, shaders, textures, etc.)
    void init() override;

    // Called every frame to update logic (e.g., rotations, animations)
    void update(float deltaTime) override;

    // Called every frame to draw objects
    void render() override;

    // Scene display name (helpful when switching scenes)
    std::string name() const override { return "SpotLight Demo Scene"; }

private:
    // OpenGL object handles
    unsigned int VAO, VBO;

    // Textures (diffuse = color, specular = shininess highlights)
    unsigned int diffuseMap, specularMap;

    // The shader program for this scene
    Shader* lightShader;

    // Fixed positions of cubes in the scene
    glm::vec3 cubePositions[10];

    // Animation control for cube rotations
    float rotationAngle;
    float rotationSpeed;

    // Utility function to load textures from file
    unsigned int loadTexture(const char* path);
};