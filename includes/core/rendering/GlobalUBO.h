#pragma once
#include <glm/glm.hpp>
#include "core/LightManager.h"

class GlobalUBO
{
public:
    GlobalUBO();
    ~GlobalUBO();

    // disable copying to prevent double deletion of GPU buffer
    GlobalUBO(const GlobalUBO&) = delete;
    GlobalUBO& operator=(const GlobalUBO&) = delete;

    void Bind(int bindingPoint = 0);
    void Upload(const LightUBO& data);

private:
    GLuint uboID = 0;
};