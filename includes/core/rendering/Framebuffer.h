#pragma once
#include <thirdparty/glad/glad.h>

class Framebuffer
{
public:
	Framebuffer(unsigned int width, unsigned int height, bool withDepth = true, bool multiSampled = false);
	~Framebuffer();

	void Bind() const;
	static void Unbind();
	void Resize(unsigned int w, unsigned int h);

	GLuint GetColorTexture() const { return colorTexture; }
	unsigned int Width() const { return width; }
	unsigned int Height() const { return height; }

	bool multiSampled = true;
	unsigned int intermediateFBO = 0;
	void ResolveToScreen();
	GLuint GetIntermediateTexture() const { return multiSampled ? screenTexture : colorTexture; }
	
private:
	
	void CreateResources();
	
	GLuint screenTexture = 0;
	GLuint fbo = 0;
	GLuint colorTexture = 0; // could be MSAA or standard
	GLuint rbo = 0;
	unsigned int width = 0, height = 0;
	bool depth = true;
};