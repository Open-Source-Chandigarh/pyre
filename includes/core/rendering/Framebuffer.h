#pragma once
#include <thirdparty/glad/glad.h>
#include <vector>

class Framebuffer
{
public:
	Framebuffer(unsigned int width, unsigned int height, bool withDepth = true, 
				bool multiSampled = false, bool withColor = true, unsigned int textureLayers = 1, 
				bool isCubeMap = false, unsigned int numColorAttachments = 1);
	~Framebuffer();

	void Bind() const;
	static void Unbind();
	void Resize(unsigned int w, unsigned int h);

	GLuint GetColorTexture(unsigned int index = 0) const 
	{ 
		if (index >= colorTextures.size()) return 0;
        return colorTextures[index]; 
	}

	GLuint GetDepthTexture() const { return depthTexture; }
	GLuint GetID() const { return fbo; }
	unsigned int Width() const { return width; }
	unsigned int Height() const { return height; }

	bool multiSampled = true;
	unsigned int intermediateFBO = 0;
	void ResolveToScreen();
	
	GLuint GetIntermediateTexture(unsigned int index = 0) const 
    { 
        if (multiSampled) 
        {
            if (index >= screenTextures.size()) return 0;
        	return screenTextures[index];
        }
        else return GetColorTexture(index);
    }
	
private:
	
	void CreateResources();
	
	std::vector<unsigned int> screenTextures;
	GLuint fbo = 0;
	std::vector<unsigned int> colorTextures; // could be MSAA or standard (can have more than 1 color attachments like one for color and one for brightness)
	GLuint depthTexture = 0;
	GLuint rbo = 0;
	unsigned int width = 0, height = 0;
	unsigned int textureLayers;
	unsigned int numColorAttachments;
	bool depth = true;
	bool color = true;
	bool isCubeMap = false;
};