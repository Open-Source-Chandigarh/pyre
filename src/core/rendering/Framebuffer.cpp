#include "core/rendering/Framebuffer.h"
#include <iostream>

Framebuffer::Framebuffer(unsigned int w, unsigned int h, bool withDepth, bool multiSampled, bool withColor, 
    unsigned int textureLayers)
    : width(w), height(h), depth(withDepth), multiSampled(multiSampled), color(withColor), textureLayers(textureLayers)
{
    CreateResources();
}

Framebuffer::~Framebuffer()
{
    if (rbo) glDeleteRenderbuffers(1, &rbo);
    if (colorTexture) glDeleteTextures(1, &colorTexture);
    if (depthTexture) glDeleteTextures(1, &depthTexture);
    if (fbo) glDeleteFramebuffers(1, &fbo);
}

void Framebuffer::CreateResources()
{
    // cleanup if existing 
    if (rbo) { glDeleteRenderbuffers(1, &rbo); rbo = 0; }
    if (colorTexture) { glDeleteTextures(1, &colorTexture); colorTexture = 0; }
    if (fbo) { glDeleteFramebuffers(1, &fbo); fbo = 0; }

    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    // color texture
    glGenTextures(1, &colorTexture);

    if(color)
    {
        if(multiSampled)
        {
            glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, colorTexture);
            glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 4, GL_RGB16F, width, height, GL_TRUE);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, colorTexture, 0);
        }
        else
        {
            glBindTexture(GL_TEXTURE_2D, colorTexture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, nullptr);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            // clamp to edge to avoid seams in post processing
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTexture, 0);
        }
    }
    else
    {
        // explicitly tell opengl we are not rendering color data
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
    }

    if (depth)
    {
        // if we have no color, we assume this is a shadow map
        // shadow maps need a depth texture, not a renderbuffer (rbo)
        if(!color)
        {   
            // shadow map case (depth only)
            glGenTextures(1, &depthTexture);
            
            // check if we need a texture array (CSM) or standard texture
            if (textureLayers > 1) 
            {
                glBindTexture(GL_TEXTURE_2D_ARRAY, depthTexture);
                glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT32F, width, 
                    height, textureLayers, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

                glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER); 
                glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER); 
                
                constexpr float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
                glTexParameterfv(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BORDER_COLOR, borderColor);
                
                // use glFramebufferTexture (not 2D) to attach the entire array volume
                // this allows the geometry shader to select the layer via gl_Layer
                glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthTexture, 0);
            }
            else
            {
                // standard 2D shadow map
                glBindTexture(GL_TEXTURE_2D, depthTexture);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
                
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
                
                constexpr float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
                glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
                
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTexture, 0);
            }
        }
        else
        {
            glGenRenderbuffers(1, &rbo);
            glBindRenderbuffer(GL_RENDERBUFFER, rbo);
            if(multiSampled)
            {
                glRenderbufferStorageMultisample(GL_RENDERBUFFER, 4, GL_DEPTH24_STENCIL8, width, height);
            }
            else
            {
                glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
            }
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);
        }
    }

    if(multiSampled)
    {
        glGenFramebuffers(1, &intermediateFBO);
        glBindFramebuffer(GL_FRAMEBUFFER, intermediateFBO);

        glGenTextures(1, &screenTexture);
        glBindTexture(GL_TEXTURE_2D, screenTexture);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, NULL);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, screenTexture, 0);
    }

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cerr << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Framebuffer::ResolveToScreen()
{
    if(!multiSampled) return;

    glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, intermediateFBO);

    glBlitFramebuffer(0, 0, width, height, 0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_NEAREST);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Framebuffer::Bind() const
{
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glViewport(0, 0, width, height);
}

void Framebuffer::Unbind()
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    // caller must set viewport for default framebuffer (Window::FramebufferSizeCallback already does)
}

void Framebuffer::Resize(unsigned int w, unsigned int h)
{
    if (w == width && h == height) return;
    width = w; height = h;
    CreateResources();
}