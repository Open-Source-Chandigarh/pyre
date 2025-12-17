#include <glad/glad.h>
#include <iostream>
#include "core/rendering/GlobalUBO.h"

GlobalUBO::GlobalUBO()
{
    glGenBuffers(1, &uboID);
    glBindBuffer(GL_UNIFORM_BUFFER, uboID);

    glBufferData(GL_UNIFORM_BUFFER, sizeof(LightUBO), nullptr, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, uboID);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

GlobalUBO::~GlobalUBO()
{
    if (uboID) glDeleteBuffers(1, &uboID);
}

void GlobalUBO::Bind(int bindingPoint)
{
    glBindBufferBase(GL_UNIFORM_BUFFER, bindingPoint, uboID);
}

void GlobalUBO::Upload(const LightUBO &data)
{
    glBindBuffer(GL_UNIFORM_BUFFER, uboID);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(LightUBO), &data);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}