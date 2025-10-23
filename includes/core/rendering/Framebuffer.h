#pragma once
#include <thirdparty/glad/glad.h>

class Framebuffer
{
public:
	Framebuffer(unsigned int width, unsigned int height, bool withDepth = true);
	~Framebuffer();

	void Bind() const;
	static void Unbind();
	void Resize(unsigned int w, unsigned int h);

	GLuint GetColorTexture() const { return colorTexture; }
	unsigned int Width() const { return width; }
	unsigned int Height() const { return height; }

private:

	void CreateResources();

	GLuint fbo = 0;
	GLuint colorTexture = 0;
	GLuint rbo = 0;
	unsigned int width = 0, height = 0;
	bool depth = true;
};