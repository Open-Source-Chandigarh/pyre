#include <thirdparty/glad/glad.h>
#include <iostream>
#include "core/postprocessing/PostProcessingPipeline.h"
#include "core/ResourceManager.h"
#include "core/postprocessing/GenericPostEffect.h"

PostProcessingPipeline::PostProcessingPipeline(unsigned int w, unsigned int h)
    : width(w), height(h)
{
    pingpong[0] = std::make_unique<Framebuffer>(width, height, false); // no depth for pingpong
    pingpong[1] = std::make_unique<Framebuffer>(width, height, false);
    EnsureQuad();

    // make sure simple texture shader is loaded (used by DrawToScreen)
    ResourceManager::LoadShader("simpleTex", 
        "shaders/common/simpleTexture.vs", "shaders/common/simpleTexture.fs");

    ResourceManager::LoadShader("post_invert", 
        "shaders/common/simpleTexture.vs", "shaders/postprocessing/inversion.fs");
    ResourceManager::LoadShader("post_grayscale", 
        "shaders/common/simpleTexture.vs", "shaders/postprocessing/grayscale.fs");
    ResourceManager::LoadShader("post_sharpen",
        "shaders/common/simpleTexture.vs", "shaders/postprocessing/sharpen.fs");

    ResourceManager::LoadShader("post_gamma", 
        "shaders/common/simpleTexture.vs", "shaders/postprocessing/gamma.fs");

    ResourceManager::LoadShader("post_hdr", 
        "shaders/common/simpleTexture.vs", "shaders/postprocessing/hdr.fs");
}

void PostProcessingPipeline::EnsureQuad()
{
    if (quadVAO != 0) return;

    float quadVertices[] = {
        // positions   // texCoords
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,

        -1.0f,  1.0f,  0.0f, 1.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f
    };

    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 
        (void*)(2 * sizeof(float)));
    glBindVertexArray(0);
}

GLuint PostProcessingPipeline::Apply(GLuint inputTex)
{
    if (effects.empty()) return inputTex;

    EnsureQuad();

    GLuint currentInput = inputTex;
    int ping = 0;
    for (size_t i = 0; i < effects.size(); i++)
    {
        Framebuffer& outFbo = *pingpong[ping];
        effects[i]->Apply(currentInput, outFbo, quadVAO);
        currentInput = outFbo.GetColorTexture();
        ping = 1 - ping; // flip to pong
    }

    // currentInput now contains final texture (texture ID)
    return currentInput;
}

void PostProcessingPipeline::DrawToScreen(GLuint texture)
{
    // bind default framebuffer
    Framebuffer::Unbind();
    glDisable(GL_DEPTH_TEST);

    auto shader = ResourceManager::GetShader("simpleTex");
    if (!shader) return;
    shader->use();
    shader->setInt("screenTexture", 0);
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);

    EnsureQuad();
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

void PostProcessingPipeline::Resize(unsigned int w, unsigned int h)
{
    width = w; height = h;
    pingpong[0]->Resize(w, h);
    pingpong[1]->Resize(w, h);
}

std::shared_ptr<PostEffect> PostProcessingPipeline::AddEffectFromShader(
    const std::string& shaderKey,
    std::function<void(Shader&)> setter)
{
    // ensure shader is already loaded by ResourceManager 
    auto shader = ResourceManager::GetShader(shaderKey);
    if (!shader) {
        std::cerr << "PostProcessingPipeline::AddEffectFromShader: shader '" << shaderKey
            << "' not found. Make sure to call ResourceManager::LoadShader earlier.\n";
        return nullptr;
    }
    auto effect = std::make_shared<GenericPostEffect>(shader, std::move(setter));
    effects.push_back(effect);
    return effect;
}

std::shared_ptr<PostEffect> PostProcessingPipeline::AddEffectFromShader(
    std::shared_ptr<Shader> shader,
    std::function<void(Shader&)> setter)
{
    if (!shader) {
        std::cerr << "PostProcessingPipeline::AddEffectFromShader: null shader\n";
        return nullptr;
    }
    auto effect = std::make_shared<GenericPostEffect>(shader, std::move(setter));
    effects.push_back(effect);
    return effect;
}

// Convenience wrappers

std::shared_ptr<PostEffect> PostProcessingPipeline::AddInversion()
{
    return AddEffectFromShader("post_invert");
}

std::shared_ptr<PostEffect> PostProcessingPipeline::AddGrayscale()
{
    return AddEffectFromShader("post_grayscale");
}

std::shared_ptr<PostEffect> PostProcessingPipeline::AddSharpen(float strength)
{
    return AddEffectFromShader("post_sharpen", [strength](Shader& s) {
        s.setFloat("strength", strength);
        });
}

std::shared_ptr<PostEffect> PostProcessingPipeline::AddGammaCorrection(float gammaVal)
{
    return AddEffectFromShader("post_gamma", [gammaVal](Shader& s) {
        s.setFloat("gamma", gammaVal);
    });
}

std::shared_ptr<PostEffect> PostProcessingPipeline::AddToneMapping(float exposure)
{
    return AddEffectFromShader("post_hdr", [exposure](Shader& s) {
        s.setFloat("exposure", exposure);
    });
}