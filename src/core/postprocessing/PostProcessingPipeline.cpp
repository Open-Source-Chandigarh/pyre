#include <thirdparty/glad/glad.h>
#include "core/postprocessing/PostProcessingPipeline.h"
#include "core/ResourceManager.h"

PostProcessingPipeline::PostProcessingPipeline(unsigned int w, unsigned int h)
    : width(w), height(h)
{
    pingpong[0] = std::make_unique<Framebuffer>(width, height, false); // no depth for pingpong
    pingpong[1] = std::make_unique<Framebuffer>(width, height, false);
    EnsureQuad();

    // make sure simple texture shader is loaded (used by DrawToScreen)
    ResourceManager::LoadShader("simpleTex", "shaders/simpleTexture.vs", "shaders/simpleTexture.fs");
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