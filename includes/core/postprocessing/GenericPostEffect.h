#pragma once
#include <memory>
#include <functional>
#include "core/postprocessing/postEffect.h"

class Shader; // forward

class GenericPostEffect : public PostEffect
{
public:
    // shader may be nullptr; Apply will be a no-op if shader missing
    GenericPostEffect(std::shared_ptr<Shader> shader,
        std::function<void(Shader&)> uniformSetter = {});

    void Apply(GLuint inputTexture, Framebuffer& output, GLuint quadVAO) override;

    std::shared_ptr<Shader> GetShader() { return shader; }

private:
    std::shared_ptr<Shader> shader;
    std::function<void(Shader&)> setter;
};