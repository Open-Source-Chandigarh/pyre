#pragma once
#include <vector>
#include <memory>
#include <functional>
#include <string>
#include "core/rendering/Framebuffer.h"
#include "core/postprocessing/postEffect.h"

class Shader; // forward

class PostProcessingPipeline
{
public:
    PostProcessingPipeline(unsigned int width, unsigned int height);
    ~PostProcessingPipeline() = default;

    template<typename T, typename... Args>
    void AddEffect(Args&&... args)
    {
        effects.push_back(std::make_shared<T>(std::forward<Args>(args)...));
    }

    // add effect by shader key (ResourceManager::GetShader must return shader)
    std::shared_ptr<PostEffect> AddEffectFromShader(const std::string& shaderKey,
        std::function<void(Shader&)> setter = {});

    // Overload: add effect from an existing shader pointer
    std::shared_ptr<PostEffect> AddEffectFromShader(std::shared_ptr<Shader> shader,
        std::function<void(Shader&)> setter = {});

    std::shared_ptr<PostEffect> AddInversion();
    std::shared_ptr<PostEffect> AddGrayscale();
    std::shared_ptr<PostEffect> AddSharpen(float strength);
    std::shared_ptr<PostEffect> AddGammaCorrection(float gammaVal);
    std::shared_ptr<PostEffect> AddToneMapping(float exposure);
    void AddBloom() { bloomEnabled = true; }
    GLuint PerformBlur(GLuint inputTexture, int iterations = 10);

    // Run all effects on inputTex. Returns GLuint of final texture.
    GLuint Apply(GLuint inputTex, GLuint brightnessTex = 0);

    // Draw a given texture directly to default framebuffer using the internal quad + simpleTex shader
    void DrawToScreen(GLuint texture);

    // Handle resize
    void Resize(unsigned int w, unsigned int h);
    unsigned int blurredBloomTexture = 0;

private:
    std::unique_ptr<Framebuffer> pingpong[2];
    std::unique_ptr<Framebuffer> bloomBuffer;
    std::vector<std::shared_ptr<PostEffect>> effects;
    GLuint quadVAO = 0, quadVBO = 0;
    unsigned int width = 0, height = 0;
    bool bloomEnabled = false;

    void EnsureQuad();
};