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
    glm::vec3 defaultDiffuseColor = glm::vec3(0.8f);
    float defaultShininess = 32.0f;
    float defaultReflectivity = 0.0f;
    bool outlineEnabled = false;
    glm::vec3 outlineColor = glm::vec3(1.0f);
    bool isTransparent = false;
    CullMode cullMode = CullMode::Back;
    bool showNormals = false;

    void ApplyToShader(Shader& shader) const
    {
        shader.use();

        // Initialize all present flags to 0 if they actually exist.
        // (Do not set them to 0 if shader doesn't declare them)
        const char* presentUniforms[] = {
            "material_diffuse_present",
            "material_specular_present",
            "material_skybox_present"
        };

        // Explicitly set skybox slot using constant
        if (shader.hasUniform("material_skybox")) {
            shader.setInt("material_skybox", Bindings::TEX_SLOT_SKYBOX);
        }
        
        for (auto& p : presentUniforms)
            if (shader.hasUniform(p))
                shader.setInt(p, 0);   // default OFF

        // Bind textures & set *_present = 1 for those actually bound.
        for (auto& kv : textures)
        {
            const std::string& name = kv.first;
            const auto& tex = kv.second;

            if (!tex) continue;
            // shader doesn't want this texture, skip
            if (!shader.hasUniform(name)) continue;  

            GLenum target = (tex->type == TextureType::TEX_CUBEMAP) ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D;
            int slot = -1;

            // Map texture types to fixed binding slots
            if (tex->type == TextureType::TEX_DIFFUSE) 
            {
                slot = Bindings::TEX_SLOT_DIFFUSE;
            }
            else if (tex->type == TextureType::TEX_SPECULAR) 
            {
                slot = Bindings::TEX_SLOT_SPECULAR;
            }
            else if (name == "material_skybox") 
            {
                slot = Bindings::TEX_SLOT_SKYBOX;
            }

            // Only bind if we have a valid slot
            if (slot != -1)
            {
                glActiveTexture(GL_TEXTURE0 + slot);
                glBindTexture(target, tex->ID);
                shader.setInt(name, slot);

                // set presence flag
                const std::string presentName = name + "_present";
                if (shader.hasUniform(presentName))
                    shader.setInt(presentName, 1);
            }
        }

        // Push float & vec3 uniforms
        for (auto& kv : floats)
            if (shader.hasUniform(kv.first))
                shader.setFloat(kv.first, kv.second);

        for (auto& kv : vec3s)
            if (shader.hasUniform(kv.first))
                shader.setVec3(kv.first, kv.second);

        // Fallback values if not overridden by textures
        if (shader.hasUniform("material_diffuseColor"))
        {
            if (vec3s.count("material_diffuseColor"))
                shader.setVec3("material_diffuseColor", vec3s.at("material_diffuseColor"));
            else
                shader.setVec3("material_diffuseColor", defaultDiffuseColor);
        }

        if (shader.hasUniform("material_specularColor"))
        {
            if (vec3s.count("material_specularColor"))
                shader.setVec3("material_specularColor", vec3s.at("material_specularColor"));
            else
                shader.setVec3("material_specularColor", glm::vec3(1.0f)); // good default
        }

        if (shader.hasUniform("material_shininess"))
        {
            if (floats.count("material_shininess"))
                shader.setFloat("material_shininess", floats.at("material_shininess"));
            else
                shader.setFloat("material_shininess", defaultShininess);
        }

        if(shader.hasUniform("material_reflectivity"))
        {
            if(floats.count("material_reflectivity"))
                shader.setFloat("material_reflectivity", floats.at("material_reflectivity"));
            else
                shader.setFloat("material_reflectivity", defaultReflectivity);
        }

        glActiveTexture(GL_TEXTURE0);
    }
};