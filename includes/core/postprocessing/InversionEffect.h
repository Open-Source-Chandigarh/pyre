#pragma once
#include <memory>
#include "helpers/shaderClass.h"
#include "core/postprocessing/postEffect.h"

class InversionEffect : public PostEffect
{
public:
    InversionEffect();
    void Apply(GLuint inputTexture, Framebuffer& output, GLuint quadVAO) override;

private:
    std::shared_ptr<Shader> shader;
};