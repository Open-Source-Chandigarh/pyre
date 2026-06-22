#pragma once
#include "core/Constants.h"
#include "core/rendering/Texture.h"
#include "helpers/Shader.h"
#include <cstddef>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

enum class CullMode
{
    Back,
    Front,
    None
};

// Material : describes surface appearance and references textures
struct Material
{
    std::unordered_map<std::string, std::shared_ptr<Texture>> textures;
    std::unordered_map<std::string, float> floats;
    std::unordered_map<std::string, glm::vec3> vec3s;
    std::unordered_map<std::string, bool> bools;
    bool isTransparent = false;
    CullMode cullMode = CullMode::Back;

    bool GetBool(const std::string &name, bool defaultVal = false) const
    {
        auto it = bools.find(name);
        return (it != bools.end()) ? it->second : defaultVal;
    }

    glm::vec3 GetVec3(const std::string &name, glm::vec3 defaultVal = glm::vec3(1.0f)) const
    {
        auto it = vec3s.find(name);
        return (it != vec3s.end()) ? it->second : defaultVal;
    }

    float GetFloat(const std::string &name, float defaultVal = 0.0f) const
    {
        auto it = floats.find(name);
        return (it != floats.end()) ? it->second : defaultVal;
    }

    void SetOutline(bool enabled, glm::vec3 color = glm::vec3(1.0f), float bloomFactor = 0.0f,
                    float outlineThickness = 0.05f)
    {
        bools["outlineEnabled"] = enabled;
        vec3s["outlineColor"] = color;
        floats["bloomFactor"] = bloomFactor;
        floats["outlineThickness"] = outlineThickness;
    }

    void SetShadows(bool enabled)
    {
        bools["castShadows"] = enabled;
    }

    void SetWireframe(bool enabled)
    {
        bools["wireframe"] = enabled;
    }

    void ApplyToShader(Shader &shader, bool isOverride = false) const
    {
        shader.use();

        // force samplers to correct slots to prevent collision at slot 0
        shader.setInt("material_diffuse", Bindings::TEX_SLOT_DIFFUSE);
        shader.setInt("material_specular", Bindings::TEX_SLOT_SPECULAR);
        shader.setInt("material_normal", Bindings::TEX_SLOT_NORMAL);
        shader.setInt("material_metallic", Bindings::TEX_SLOT_METALLIC);
        shader.setInt("material_roughness", Bindings::TEX_SLOT_ROUGHNESS);
        shader.setInt("material_ao", Bindings::TEX_SLOT_AO);
        shader.setInt("material_displacement", Bindings::TEX_SLOT_DISPLACEMENT);
        shader.setInt("material_emissive", Bindings::TEX_SLOT_EMISSIVE);
        shader.setInt("material_skybox", Bindings::TEX_SLOT_SKYBOX);

        // reset present flags to 0 if no override (default state)
        if (!isOverride)
        {
            if (shader.hasUniform("material_diffuse_present"))
                shader.setInt("material_diffuse_present", 0);
            if (shader.hasUniform("material_specular_present"))
                shader.setInt("material_specular_present", 0);
            if (shader.hasUniform("material_normal_present"))
                shader.setInt("material_normal_present", 0);
            if (shader.hasUniform("material_displacement_present"))
                shader.setInt("material_displacement_present", 0);
            if (shader.hasUniform("material_metallic_present"))
                shader.setInt("material_metallic_present", 0);
            if (shader.hasUniform("material_roughness_present"))
                shader.setInt("material_roughness_present", 0);
            if (shader.hasUniform("material_emissive_present"))
                shader.setInt("material_emissive_present", 0);
            if (shader.hasUniform("material_ao_present"))
                shader.setInt("material_ao_present", 0);
            if (shader.hasUniform("material_skybox_present"))
                shader.setInt("material_skybox_present", 0);

            if (shader.hasUniform("material_shininess"))
                shader.setFloat("material_shininess", 32.0f);
            if (shader.hasUniform("material_reflectivity"))
                shader.setFloat("material_reflectivity", 0.0f);
            if (shader.hasUniform("material_heightScale"))
                shader.setFloat("material_heightScale", 0.1f);

            if (shader.hasUniform("material_diffuseColor"))
                shader.setVec3("material_diffuseColor", glm::vec3(1.0f));
            if (shader.hasUniform("material_specularColor"))
                shader.setVec3("material_specularColor", glm::vec3(0.0f));
            if (shader.hasUniform("material_emissiveColor"))
                shader.setVec3("material_emissiveColor", glm::vec3(0.0f));
            if (shader.hasUniform("emission_tint"))
                shader.setVec3("emission_tint", glm::vec3(1.0f));
        }

        // iterate available textures and bind based on type
        for (const auto &kv : textures)
        {
            const std::string &uniformName = kv.first;
            std::shared_ptr<Texture> tex = kv.second;
            if (!tex)
            {
                // if this is an override and the texture was explicitly removed, disable it
                if (isOverride)
                {
                    std::string flagName = uniformName + "_present";
                    if (shader.hasUniform(flagName))
                        shader.setInt(flagName, 0);
                }
                continue;
            }

            // determine slot based on type
            int slot = -1;
            switch (tex->type)
            {
            case TextureType::TEX_DIFFUSE:
                slot = Bindings::TEX_SLOT_DIFFUSE;
                break;
            case TextureType::TEX_SPECULAR:
                slot = Bindings::TEX_SLOT_SPECULAR;
                break;
            case TextureType::TEX_NORMAL:
                slot = Bindings::TEX_SLOT_NORMAL;
                break;
            case TextureType::TEX_METALLIC:
                slot = Bindings::TEX_SLOT_METALLIC;
                break;
            case TextureType::TEX_ROUGHNESS:
                slot = Bindings::TEX_SLOT_ROUGHNESS;
                break;
            case TextureType::TEX_AO:
                slot = Bindings::TEX_SLOT_AO;
                break;
            case TextureType::TEX_DISPLACEMENT:
                slot = Bindings::TEX_SLOT_DISPLACEMENT;
                break;
            case TextureType::TEX_EMISSIVE:
                slot = Bindings::TEX_SLOT_EMISSIVE;
                break;
            case TextureType::TEX_CUBEMAP:
            case TextureType::TEX_ENVIRONMENT:
                slot = Bindings::TEX_SLOT_SKYBOX;
                break;
            default:
                slot = 0;
                break;
            }

            if (slot != -1)
            {
                // bind texture
                GLenum target = (tex->type == TextureType::TEX_CUBEMAP || tex->type == TextureType::TEX_ENVIRONMENT)
                                    ? GL_TEXTURE_CUBE_MAP
                                    : GL_TEXTURE_2D;

                glActiveTexture(GL_TEXTURE0 + slot);
                glBindTexture(target, tex->ID);

                // update shader uniform to point to this slot
                shader.setInt(uniformName, slot);

                // if there is a matching "present" flag, enable it
                std::string flagName = uniformName + "_present";
                if (shader.hasUniform(flagName))
                    shader.setInt(flagName, 1);
            }
        }

        // apply uniforms
        for (auto &kv : floats)
            if (shader.hasUniform(kv.first))
                shader.setFloat(kv.first, kv.second);
        for (auto &kv : vec3s)
            if (shader.hasUniform(kv.first))
                shader.setVec3(kv.first, kv.second);
        for (auto &kv : bools)
            if (shader.hasUniform(kv.first))
                shader.setBool(kv.first, kv.second);

        glActiveTexture(GL_TEXTURE0);
    }
};