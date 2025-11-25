#include <glad/glad.h>
#include <iostream>
#include "core/rendering/GlobalUBO.h"

static GLuint g_ubo = 0;

void CreateGlobalUBO(){
	if (g_ubo) return;
	glGenBuffers(1, &g_ubo);
	glBindBuffer(GL_UNIFORM_BUFFER, g_ubo);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(LightUBO), nullptr, GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_UNIFORM_BUFFER, 0, g_ubo); // binding point 0
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}


void UpdateGlobalUBO(const LightUBO& data){
    if (!g_ubo) {
        std::cerr << "GlobalUBO not created\n"; return;
    }
    glBindBuffer(GL_UNIFORM_BUFFER, g_ubo);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(LightUBO), &data);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void DestroyGlobalUBO(){
    if (g_ubo) { glDeleteBuffers(1, &g_ubo); g_ubo = 0; }
}