#include "core/postprocessing/GenericPostEffect.h"
#include "core/ResourceManager.h"
#include "helpers/Shader.h"
#include <thirdparty/glad/glad.h>

GenericPostEffect::GenericPostEffect(std::shared_ptr<Shader> shader, std::function<void(Shader &)> uniformSetter)
{
    this->shader = shader;
    setter = std::move(uniformSetter);
}

void GenericPostEffect::Apply(GLuint inputTexture, Framebuffer &output, GLuint quadVAO)
{
    if (!shader)
        return;

    // Bind output FBO and prepare for a screen-space pass
    output.Bind();
    glDisable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT);

    shader->use();
    shader->setInt("scene", 0); // convention: input bound to texture slot 0

    if (uniformSetter)
        uniformSetter(*shader); // let caller set uniforms / bind extra textures

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, inputTexture);

    // draw full-screen quad (VAO provided by pipeline)
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);

    Framebuffer::Unbind();
}