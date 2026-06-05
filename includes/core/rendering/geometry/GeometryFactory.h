#pragma once
#include "core/rendering/Mesh.h"
#include <glm/glm.hpp>
#include <vector>

namespace GeometryFactory
{
std::shared_ptr<Mesh> CreateCube(float size = 1.0f);
std::shared_ptr<Mesh> CreateSkyboxCube(float size = 1.0f);
std::shared_ptr<Mesh> CreatePlane(float size = 1.0f);
std::shared_ptr<Mesh> CreateSphere(float radius = 1.0f, int segments = 32, int rings = 16);
std::shared_ptr<Mesh> CreateCylinder(float radius = 1.0f, float height = 2.0f, int segments = 32);
std::shared_ptr<Mesh> CreateCone(float radius = 1.0f, float height = 2.0f, int segments = 32);
std::shared_ptr<Mesh> CreateTorus(float radius = 1.0f, float tubeRadius = 0.3f, int segments = 32, int rings = 16);
} // namespace GeometryFactory