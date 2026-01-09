#include "core/UniformBuffer.h"

UniformBuffer::UniformBuffer(size_t size, unsigned int bindingPoint)
    : bufferSize(size)
{
    glGenBuffers(1, &uboID);
    glBindBuffer(GL_UNIFORM_BUFFER, uboID);

    // allocate memory on gpu (dynamic draw because we update it often)
    glBufferData(GL_UNIFORM_BUFFER, size, nullptr, GL_DYNAMIC_DRAW);
    
    // bind the buffer to the specific binding point index (0, 1, 2, etc.)
    glBindBufferBase(GL_UNIFORM_BUFFER, bindingPoint, uboID);
    
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

UniformBuffer::~UniformBuffer()
{
    if (uboID) 
    {
        glDeleteBuffers(1, &uboID);
    }
}

void UniformBuffer::UploadData(const void *data, size_t size, size_t offset)
{
    glBindBuffer(GL_UNIFORM_BUFFER, uboID);
    glBufferSubData(GL_UNIFORM_BUFFER, offset, size, data);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void UniformBuffer::Bind()
{
    glBindBuffer(GL_UNIFORM_BUFFER, uboID);
}

void UniformBuffer::Unbind()
{
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}