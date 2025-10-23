#pragma once
#include <thirdparty/glad/glad.h>
#include "core/rendering/Framebuffer.h"

class PostEffect
{
public:
	virtual ~PostEffect() = default;

	// Apply: inputTexture -> render into output FBO (output.Bind() is called inside)
	virtual void Apply(GLuint inputTexture, Framebuffer& output, GLuint quadVAO) = 0;
};