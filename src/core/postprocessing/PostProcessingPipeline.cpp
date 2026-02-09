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
    bloomBuffer = std::make_unique<Framebuffer>(width, height, false);
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

    ResourceManager::LoadShader("post_blur", 
        "shaders/common/simpleTexture.vs", "shaders/postprocessing/blur.fs");
    ResourceManager::LoadShader("post_hdr_combine", 
        "shaders/common/simpleTexture.vs", "shaders/postprocessing/hdr_combine.fs");

    // Pre-create standard effects (Disabled by default)
    AddInversion()->enabled = false;
    AddGrayscale()->enabled = false;
    AddSharpen(1.0f)->enabled = false;
    // Gamma and ToneMapping are often enabled by default in scenes, 
    // but here we just ensure they are available.
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

GLuint PostProcessingPipeline::PerformBlur(GLuint inputTex, int iterations)
{
    bool horizontal = true;
    bool first_iteration = true;
    auto shader = ResourceManager::GetShader("post_blur");

    shader->use();
    shader->setInt("scene", 0);

    for (int i = 0; i < iterations; i++)
    {
        pingpong[horizontal]->Bind();

        shader->setInt("horizontal", horizontal);
        glActiveTexture(GL_TEXTURE0);
        // if it is the very first step, read from the input.
        // otherwise, read from the pingpong buffer we wrote to in the last step.
        glBindTexture(GL_TEXTURE_2D, first_iteration ? inputTex : pingpong[!horizontal]->GetColorTexture());

        // render to quad
        glBindVertexArray(quadVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        
        // swap for next run
        horizontal = !horizontal;
        if (first_iteration) first_iteration = false;
    }

    Framebuffer::Unbind();

    // we wrote to 'horizontal' inside the loop, The loop toggles 'horizontal' at the end.
    // so the valid data is in '!horizontal'.
    return pingpong[!horizontal]->GetColorTexture();
}

GLuint PostProcessingPipeline::Apply(GLuint inputTex, GLuint brightnessTex)
{
    if (bloomEnabled && brightnessTex != 0) 
    {
        // run the blur loop using pingpong buffers
        // this leaves the result in one of the pingpong buffers
        GLuint blurredTex = PerformBlur(brightnessTex, bloomIterations);

        // we must copy 'blurredTex' into 'bloomBuffer' because the very next loop
        // will overwrite the pingpong buffers.
        bloomBuffer->Bind();
        glDisable(GL_DEPTH_TEST);
        
        auto shader = ResourceManager::GetShader("simpleTex");
        shader->use();
        shader->setInt("screenTexture", 0);
        
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, blurredTex);
        
        glBindVertexArray(quadVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        
        Framebuffer::Unbind();

        // point to the safe texture, not the reused pingpong one
        this->blurredBloomTexture = bloomBuffer->GetColorTexture();
    }
    if (effects.empty()) return inputTex;
    
    EnsureQuad();

    GLuint currentInput = inputTex;
    int ping = 0;
    for (size_t i = 0; i < effects.size(); i++)
    {
        if (!effects[i]->enabled) continue;

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
    bloomBuffer->Resize(w, h);
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
    auto e = AddEffectFromShader("post_invert");
    if (e) e->name = "Inversion";
    return e;
}

std::shared_ptr<PostEffect> PostProcessingPipeline::AddGrayscale()
{
    auto e = AddEffectFromShader("post_grayscale");
    if (e) e->name = "Grayscale";
    return e;
}

std::shared_ptr<PostEffect> PostProcessingPipeline::AddSharpen(float strength)
{
    auto e = AddEffectFromShader("post_sharpen");
    if (e) {
        e->name = "Sharpen";
        e->intensity = strength;
        e->uniformSetter = [e](Shader& s) {
            s.setFloat("strength", e->intensity);
        };
    }
    return e;
}

std::shared_ptr<PostEffect> PostProcessingPipeline::AddGammaCorrection(float gammaVal)
{
    auto e = AddEffectFromShader("post_gamma");
    if (e) {
        e->name = "Gamma Correction";
        e->intensity = gammaVal;
        e->uniformSetter = [e](Shader& s) {
            s.setFloat("gamma", e->intensity);
        };
    }
    return e;
}

std::shared_ptr<PostEffect> PostProcessingPipeline::AddToneMapping(float exposure)
{
    auto e = AddEffectFromShader("post_hdr_combine");
    if (e) {
        e->name = "Tone Mapping";
        e->intensity = exposure;
        e->uniformSetter = [this, e](Shader& s) {
            s.setInt("scene", 0);
            s.setFloat("exposure", e->intensity);
            s.setInt("bloomEnabled", this->bloomEnabled);
            
            // if bloom is active, bind the blurred texture to Slot 1
            if (this->bloomEnabled) {
                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_2D, this->blurredBloomTexture);
                s.setInt("bloomBlur", 1);
            }
            glActiveTexture(GL_TEXTURE0);
        };
    }
    return e;
}