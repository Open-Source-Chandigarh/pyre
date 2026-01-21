#pragma once

#include <thirdparty/glm/glm.hpp>

namespace Bindings 
{
    // Uniform Buffer Binding Points
    constexpr int UBO_CAMERA        = 0;
    constexpr int UBO_LIGHTS        = 1;
    constexpr int UBO_CSM_SHADOWS   = 2;
    constexpr int UBO_POINT_SHADOWS = 3;

    // Texture Units / Slots
    constexpr int TEX_SLOT_DIFFUSE      = 0;
    constexpr int TEX_SLOT_SPECULAR     = 1;
    constexpr int TEX_SLOT_NORMAL       = 2; // Reserved for future use
    constexpr int TEX_SLOT_CSM_SHADOW   = 10;
    constexpr int TEX_SLOT_POINT_SHADOW = 11;
    constexpr int TEX_SLOT_SKYBOX       = 15;
}

//Camera Defaults
namespace CameraDefaults
{
    inline constexpr float YAW   = -90.0f;
    inline constexpr float PITCH = 0.0f;
    inline constexpr float SPEED = 10.0f;
    inline constexpr float SENSITIVITY = 0.1f;
    inline constexpr float ZOOM  = 45.0f;

    inline constexpr float NEAR_PLANE = 0.1f;
    inline constexpr float FAR_PLANE  = 100.0f;

    inline constexpr glm::vec3 POSITION = glm::vec3(0.0f, 0.0f, 3.0f);
    inline constexpr glm::vec3 UP       = glm::vec3(0.0f, 1.0f, 0.0f);
    inline constexpr glm::vec3 FRONT    = glm::vec3(0.0f, 0.0f, -1.0f);
}
