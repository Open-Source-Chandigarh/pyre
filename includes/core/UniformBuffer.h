#pragma once
#include <glad/glad.h>
#include <cstddef>

class UniformBuffer
{
public:
    // constructor initializes the buffer with a specific size and binding point
    // usage: UniformBuffer ubo(sizeof(MyStruct), 0);
    UniformBuffer(size_t size, unsigned int bindingPoint);
    ~UniformBuffer();

    // disable copying
    UniformBuffer(const UniformBuffer&) = delete;
    UniformBuffer& operator=(const UniformBuffer&) = delete;

    // uploads data to the gpu
    // offset allows updating just a part of the buffer if needed
    void UploadData(const void *data, size_t size, size_t offset = 0);

    void Bind();
    void Unbind();

private:
    GLuint uboID = 0;
    size_t bufferSize = 0;
};