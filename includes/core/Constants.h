#pragma once

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
    constexpr int TEX_SLOT_NORMAL       = 2;
    constexpr int TEX_SLOT_DISPLACEMENT = 3;
    constexpr int TEX_SLOT_GBUFFER_POSITION = 5;
    constexpr int TEX_SLOT_GBUFFER_NORMAL = 6;
    constexpr int TEX_SLOT_GBUFFER_ALBEDO = 7;
    constexpr int TEX_SLOT_SSAO_NOISE = 8;

    constexpr int TEX_SLOT_CSM_SHADOW   = 10;
    constexpr int TEX_SLOT_POINT_SHADOW = 11;
    constexpr int TEX_SLOT_SKYBOX       = 15;
}