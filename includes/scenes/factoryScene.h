#pragma once
#include <thirdparty/glm/glm.hpp>
#include <vector>
#include "Scene.h"
#include "helpers/Shader.h"
#include "helpers/camera.h"
#include "core/Window.h"
#include "application/appState.h"
#include "core/rendering/Mesh.h"
#include "core/rendering/Renderer.h"
#include "core/LightManager.h"
#include "core/Entity.h"
#include "core/rendering/Framebuffer.h"
#include "core/postprocessing/PostProcessingPipeline.h"
#include "core/rendering/Material.h"

class FactoryScene : public Scene 
{
public:
    FactoryScene();
    ~FactoryScene();

    // called once when the scene is created (setup VAOs, VBOs, shaders, textures, etc.)
    void Init(AppState &appState) override;

    // called every frame to update logic (e.g., rotations, animations)
    void Update(AppState &appState) override;

    // Called when this scene is activated
    void OnActivate(AppState &appState) override;

    std::string Name() const override { return "Factory Demo Scene"; }

private:
    std::shared_ptr<Texture> diffuseMap, specularMap;
    std::shared_ptr<Shader> shader;
    glm::vec3 cubePositions[10];

    std::shared_ptr<Mesh> mesh[10];
    std::shared_ptr<Mesh> skyMesh;

    float rotationAngle;
    float rotationSpeed;
};