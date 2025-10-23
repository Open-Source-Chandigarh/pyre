#include "core/postprocessing/InversionEffect.h"
#include "core/ResourceManager.h"
#include <thirdparty/glad/glad.h>

InversionEffect::InversionEffect()
{
    // load shader once (ResourceManager caches it)
    shader = ResourceManager::LoadShader("post_inversion", "shaders/simpleTexture.vs", "shaders/inversion.fs");
}

void InversionEffect::Apply(GLuint inputTexture, Framebuffer& output, GLuint quadVAO)
{
    if (!shader) return;

    output.Bind();
    glDisable(GL_DEPTH_TEST); // screen-space
    glClear(GL_COLOR_BUFFER_BIT); // optional (not strictly required)

    shader->use();
    shader->setInt("scene", 0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, inputTexture);

    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6); // our quad uses 2 triangles (6 verts)
    glBindVertexArray(0);

    Framebuffer::Unbind();
}