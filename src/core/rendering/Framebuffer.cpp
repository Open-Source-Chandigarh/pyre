#include "core/rendering/Framebuffer.h"
#include <iostream>

Framebuffer::Framebuffer(unsigned int w, unsigned int h, bool withDepth, bool multiSampled, bool withColor,
                         unsigned int textureLayers, bool isCubeMap, unsigned int numColorAttachments)
    : width(w), height(h), depth(withDepth), multiSampled(multiSampled), color(withColor), textureLayers(textureLayers),
      isCubeMap(isCubeMap), numColorAttachments(numColorAttachments)
{
    CreateResources();
}

Framebuffer::~Framebuffer()
{
    if (rbo)
        glDeleteRenderbuffers(1, &rbo);
    for (auto t : colorTextures)
        glDeleteTextures(1, &t);
    if (depthTexture)
        glDeleteTextures(1, &depthTexture);
    if (fbo)
        glDeleteFramebuffers(1, &fbo);
}

void Framebuffer::CreateResources()
{
    // cleanup if existing
    if (rbo)
    {
        glDeleteRenderbuffers(1, &rbo);
        rbo = 0;
    }
    colorTextures.clear();
    screenTextures.clear();
    if (fbo)
    {
        glDeleteFramebuffers(1, &fbo);
        fbo = 0;
    }

    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    std::vector<GLenum> attachments;

    if (color)
    {
        for (unsigned int i = 0; i < numColorAttachments; i++)
        {
            unsigned int tex;
            glGenTextures(1, &tex);
            GLenum target = multiSampled ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D;
            glBindTexture(target, tex);

            GLenum internalFormat = GL_RGBA16F;
            if (multiSampled)
            {
                glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 4, internalFormat, width, height, GL_TRUE);
            }
            else
            {
                glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            }

            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, target, tex, 0);
            attachments.push_back(GL_COLOR_ATTACHMENT0 + i);
            colorTextures.push_back(tex);
        }
    }
    else
    {
        // explicitly tell opengl we are not rendering color data
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
    }

    if (!attachments.empty())
        glDrawBuffers(attachments.size(), attachments.data());

    if (depth)
    {
        // if we have no color, we assume this is a shadow map
        // shadow maps need a depth texture, not a renderbuffer (rbo)
        if (!color)
        {
            // shadow map case (depth only)
            glGenTextures(1, &depthTexture);

            // check if we need a cubemap (for omnidirectional shadows)
            if (isCubeMap && textureLayers > 0)
            {
                glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, depthTexture);
                glTexImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, 0, GL_DEPTH_COMPONENT32F, width, height, textureLayers * 6, 0,
                             GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
                glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                // Enable Hardware Shadow Comparison
                // This turns "0.5 depth" into a "50% lit" probability automatically
                glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
                glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);

                glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

                glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthTexture, 0);
            }

            // check if we need a texture array (CSM) or standard texture
            else if (textureLayers > 1)
            {
                glBindTexture(GL_TEXTURE_2D_ARRAY, depthTexture);
                glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT32F, width, height, textureLayers, 0,
                             GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

                glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
                glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

                constexpr float borderColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
                glTexParameterfv(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BORDER_COLOR, borderColor);

                // use glFramebufferTexture (not 2D) to attach the entire array volume
                // this allows the geometry shader to select the layer via gl_Layer
                glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthTexture, 0);
            }
            else
            {
                // standard 2D shadow map
                glBindTexture(GL_TEXTURE_2D, depthTexture);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT,
                             NULL);

                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTexture, 0);
            }
        }
        else
        {
            if (multiSampled)
            {
                glGenRenderbuffers(1, &rbo);
                glBindRenderbuffer(GL_RENDERBUFFER, rbo);
                glRenderbufferStorageMultisample(GL_RENDERBUFFER, 4, GL_DEPTH24_STENCIL8, width, height);
                glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);
            }
            else
            {
                glGenTextures(1, &depthTexture);
                glBindTexture(GL_TEXTURE_2D, depthTexture);

                glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, width, height, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, nullptr);

                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, depthTexture, 0);
            }
        }
    }

    if (multiSampled)
    {
        glGenFramebuffers(1, &intermediateFBO);
        glBindFramebuffer(GL_FRAMEBUFFER, intermediateFBO);

        for (unsigned int i = 0; i < numColorAttachments; i++)
        {
            unsigned int tex;
            glGenTextures(1, &tex);
            glBindTexture(GL_TEXTURE_2D, tex);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, tex, 0);
            screenTextures.push_back(tex);
        }
    }

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cerr << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Framebuffer::ResolveToScreen()
{
    if (!multiSampled)
        return;

    glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, intermediateFBO);

    for (size_t i = 0; i < colorTextures.size(); i++)
    {
        glReadBuffer(GL_COLOR_ATTACHMENT0 + i);
        glDrawBuffer(GL_COLOR_ATTACHMENT0 + i);
        glBlitFramebuffer(0, 0, width, height, 0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    }
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
    if (w == width && h == height)
        return;
    width = w;
    height = h;
    CreateResources();
}