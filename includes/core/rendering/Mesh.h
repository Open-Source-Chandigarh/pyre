#pragma once
#include <glad/glad.h>
#include <cstddef>

struct Mesh
{
	unsigned int VAO = 0;
	unsigned int VBO = 0;
    int vertexCount = 0;

    // Creates a mesh from interleaved float data (pos(3), norm(3), uv(2))
    static Mesh CreateFromData(const float* vertices, std::size_t bytes, int vertexCount);

    // Convenience: create a unit cube with the same layout
    static Mesh CreateCube();

    // Destroy GPU objects
    void Destroy();
};