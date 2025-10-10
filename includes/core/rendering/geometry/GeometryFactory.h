#pragma once
#include <vector>
#include <glm/glm.hpp>
#include "core/rendering/Mesh.h"

namespace GeometryFactory
{
    Mesh CreateCube(float size = 1.0f);
    Mesh CreatePlane(float size = 1.0f);
    Mesh CreateSphere(float radius = 1.0f, int segments = 32, int rings = 16);
    Mesh CreateCylinder(float radius = 1.0f, float height = 2.0f, int segments = 32);
    Mesh CreateCone(float radius = 1.0f, float height = 2.0f, int segments = 32);
    Mesh CreateTorus(float radius = 1.0f, float tubeRadius = 0.3f, int segments = 32, int rings = 16);
}