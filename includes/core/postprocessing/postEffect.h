#pragma once
#include <thirdparty/glad/glad.h>
#include "core/rendering/Framebuffer.h"

#include <string>

#include <functional>

class Shader;

class PostEffect
{
public:
	virtual ~PostEffect() = default;

	// Apply: inputTexture -> render into output FBO (output.Bind() is called inside)
	virtual void Apply(GLuint inputTexture, Framebuffer& output, GLuint quadVAO) = 0;

	std::string name = "Generic Effect";
	bool enabled = true;
	float intensity = 1.0f;
	std::function<void(Shader&)> uniformSetter;
};