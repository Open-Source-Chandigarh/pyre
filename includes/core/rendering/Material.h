#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <cstddef>
#include <string>
#include <vector>
#include <memory>
#include <iostream>
#include "helpers/Shader.h"
#include "core/rendering/Texture.h"
#include "core/Constants.h" 

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

    void SetOutline(bool enabled, glm::vec3 color = glm::vec3(1.0f), bool glow = false) 
    {
        bools["outlineEnabled"] = enabled;
        vec3s["outlineColor"] = color;
        bools["outlineGlow"] = glow; 
    }

    void SetShadows(bool enabled) 
    {
        bools["castShadows"] = enabled;
    }

    void SetWireframe(bool enabled) 
    {
        bools["wireframe"] = enabled;
    }

    // returns a new material where 'base' provides defaults, and 'over' overrides them.
    // if 'over' is null, returns base. If 'base' is null, returns over.
    static std::shared_ptr<Material> Mix(const std::shared_ptr<Material>& base, const std::shared_ptr<Material>& over)
    {
        if (!base && !over) return std::make_shared<Material>();
        if (!base) return over;
        if (!over) return base;

        // 1. Create a copy of the base material
        auto result = std::make_shared<Material>(*base);

        // 2. Overwrite with properties from 'over'
        for (auto&& kv : over->textures) result->textures[kv.first] = kv.second;
        for (auto&& kv : over->floats)   result->floats[kv.first] = kv.second;
        for (auto&& kv : over->vec3s)    result->vec3s[kv.first] = kv.second;
        for (auto&& kv : over->bools)    result->bools[kv.first] = kv.second;
        
        // 3. If override explicitly sets non-default pipeline states, take them.
        if(over->isTransparent) result->isTransparent = true;
        if(over->cullMode != CullMode::Back) result->cullMode = over->cullMode;

        return result;
    }

    void ApplyToShader(Shader& shader) const
    {
        shader.use();

        // force samplers to correct slots to prevent collision at slot 0
        shader.setInt("material_diffuse", Bindings::TEX_SLOT_DIFFUSE);
        shader.setInt("material_specular", Bindings::TEX_SLOT_SPECULAR);
        shader.setInt("material_normal", Bindings::TEX_SLOT_NORMAL);
        shader.setInt("material_displacement", Bindings::TEX_SLOT_DISPLACEMENT);
        shader.setInt("material_skybox", Bindings::TEX_SLOT_SKYBOX);

        // reset present flags to 0 (default state)
        if (shader.hasUniform("material_diffuse_present"))      shader.setInt("material_diffuse_present", 0);
        if (shader.hasUniform("material_specular_present"))     shader.setInt("material_specular_present", 0);
        if (shader.hasUniform("material_normal_present"))       shader.setInt("material_normal_present", 0);
        if (shader.hasUniform("material_displacement_present")) shader.setInt("material_displacement_present", 0);
        if (shader.hasUniform("material_skybox_present"))       shader.setInt("material_skybox_present", 0);

        // iterate available textures and bind based on type
        for (const auto& kv : textures)
        {
            const std::string& uniformName = kv.first;
            std::shared_ptr<Texture> tex = kv.second;
            if (!tex) continue;

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
                case TextureType::TEX_DISPLACEMENT: 
                    slot = Bindings::TEX_SLOT_DISPLACEMENT; 
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
                                ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D;

                glActiveTexture(GL_TEXTURE0 + slot);
                glBindTexture(target, tex->ID);

                // update shader uniform to point to this slot
                shader.setInt(uniformName, slot);

                // if there is a matching "present" flag, enable it
                std::string flagName = uniformName + "_present";
                if (shader.hasUniform(flagName)) shader.setInt(flagName, 1);
            }
        }

        // apply uniforms
        for (auto& kv : floats) if (shader.hasUniform(kv.first)) shader.setFloat(kv.first, kv.second);
        for (auto& kv : vec3s)  if (shader.hasUniform(kv.first)) shader.setVec3(kv.first, kv.second);
        for (auto& kv : bools)  if (shader.hasUniform(kv.first)) shader.setBool(kv.first, kv.second);

        glActiveTexture(GL_TEXTURE0);
    }
};