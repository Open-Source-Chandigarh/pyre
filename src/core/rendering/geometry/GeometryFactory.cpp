#include "core/rendering/geometry/GeometryFactory.h"
#include <array>
#include <numbers>
#include <cmath>
#include <glm/glm.hpp>

using namespace GeometryFactory;

static inline void pushVertex(std::vector<float>& v, glm::vec3 pos, glm::vec3 n, glm::vec2 uv)
{
    v.insert(v.end(), { pos.x, pos.y, pos.z, n.x, n.y, n.z, uv.x, uv.y });
}

// ------------------------------------------------------------
// CUBE
// ------------------------------------------------------------
Mesh GeometryFactory::CreateCube(float size)
{
    const float h = size * 0.5f;
    std::vector<float> data;
    std::vector<unsigned int> indices;

    glm::vec3 positions[8] = {
        {-h,-h,-h}, { h,-h,-h}, { h, h,-h}, {-h, h,-h}, // back
        {-h,-h, h}, { h,-h, h}, { h, h, h}, {-h, h, h}  // front
    };

    glm::vec3 normals[6] = {
        { 0,  0, -1}, { 0,  0,  1}, { 1,  0,  0},
        {-1,  0,  0}, { 0,  1,  0}, { 0, -1,  0}
    };

    glm::vec2 uvs[4] = { {0,0}, {1,0}, {1,1}, {0,1} };

    // Front face (0𢴐�3 style indexing)
    auto quad = [&](int a, int b, int c, int d, glm::vec3 n)
        {
            unsigned int startIndex = data.size() / 8;
            pushVertex(data, positions[a], n, uvs[0]);
            pushVertex(data, positions[b], n, uvs[1]);
            pushVertex(data, positions[c], n, uvs[2]);
            pushVertex(data, positions[d], n, uvs[3]);
            indices.insert(indices.end(), { startIndex, startIndex + 1, startIndex + 2,
                                            startIndex, startIndex + 2, startIndex + 3 });
        };

    quad(4, 5, 6, 7, normals[1]); // front
    quad(1, 0, 3, 2, normals[0]); // back
    quad(5, 1, 2, 6, normals[2]); // right
    quad(0, 4, 7, 3, normals[3]); // left
    quad(3, 7, 6, 2, normals[4]); // top
    quad(0, 1, 5, 4, normals[5]); // bottom

    return Mesh::CreateFromIndexedData(
        data.data(), data.size() * sizeof(float),
        indices.data(), indices.size() * sizeof(unsigned int),
        indices.size()
    );
}

// ------------------------------------------------------------
// PLANE
// ------------------------------------------------------------
Mesh GeometryFactory::CreatePlane(float size)
{
    float h = size * 0.5f;
    std::vector<float> data;
    std::vector<unsigned int> indices = { 0,1,2, 0,2,3 };

    pushVertex(data, { -h,0,-h }, { 0,1,0 }, { 0,0 });
    pushVertex(data, { h,0,-h }, { 0,1,0 }, { 1,0 });
    pushVertex(data, { h,0, h }, { 0,1,0 }, { 1,1 });
    pushVertex(data, { -h,0, h }, { 0,1,0 }, { 0,1 });

    return Mesh::CreateFromIndexedData(
        data.data(), data.size() * sizeof(float),
        indices.data(), indices.size() * sizeof(unsigned int),
        indices.size()
    );
}

// ------------------------------------------------------------
// SPHERE
// ------------------------------------------------------------
Mesh GeometryFactory::CreateSphere(float radius, int segments, int rings)
{
    std::vector<float> data;
    std::vector<unsigned int> indices;

    for (int y = 0; y <= rings; ++y)
    {
        float v = (float)y / rings;
        float phi = v * std::numbers::pi_v<float>;

        for (int x = 0; x <= segments; ++x)
        {
            float u = (float)x / segments;
            float theta = u * std::numbers::pi_v<float> *2.0f;

            glm::vec3 pos{
                radius * sin(phi) * cos(theta),
                radius * cos(phi),
                radius * sin(phi) * sin(theta)
            };
            glm::vec3 n = glm::normalize(pos);
            pushVertex(data, pos, n, { u, v });
        }
    }

    for (int y = 0; y < rings; ++y)
    {
        for (int x = 0; x < segments; ++x)
        {
            unsigned int i0 = y * (segments + 1) + x;
            unsigned int i1 = i0 + segments + 1;
            indices.insert(indices.end(), { i0, i1, i0 + 1, i1, i1 + 1, i0 + 1 });
        }
    }

    return Mesh::CreateFromIndexedData(
        data.data(), data.size() * sizeof(float),
        indices.data(), indices.size() * sizeof(unsigned int),
        indices.size()
    );
}

// ------------------------------------------------------------
// CYLINDER
// ------------------------------------------------------------
Mesh GeometryFactory::CreateCylinder(float radius, float height, int segments)
{
    std::vector<float> data;
    std::vector<unsigned int> indices;
    float halfH = height * 0.5f;

    // --- Side vertices ---
    for (int i = 0; i <= segments; ++i)
    {
        float theta = (i / (float)segments) * 2.0f * std::numbers::pi_v<float>;
        float x = cos(theta), z = sin(theta);
        glm::vec3 normal{ x, 0, z };

        pushVertex(data, { radius * x, -halfH, radius * z }, normal, { (float)i / segments, 0 });
        pushVertex(data, { radius * x,  halfH, radius * z }, normal, { (float)i / segments, 1 });
    }

    // --- Side indices ---
    for (int i = 0; i < segments; ++i)
    {
        unsigned int base = i * 2;
        indices.insert(indices.end(), { base, base + 1, base + 2, base + 1, base + 3, base + 2 });
    }

    // --- Bottom cap ---
    unsigned int bottomCenterIndex = data.size() / 8;
    pushVertex(data, { 0, -halfH, 0 }, { 0, -1, 0 }, { 0.5f, 0.5f });

    for (int i = 0; i <= segments; ++i)
    {
        float theta = (i / (float)segments) * 2.0f * std::numbers::pi_v<float>;
        float x = cos(theta), z = sin(theta);
        pushVertex(data, { radius * x, -halfH, radius * z }, { 0, -1, 0 }, { (x + 1) * 0.5f, (z + 1) * 0.5f });
    }

    for (int i = 0; i < segments; ++i)
    {
        indices.insert(indices.end(), {
            bottomCenterIndex,
            bottomCenterIndex + i + 1,
            bottomCenterIndex + i + 2
            });
    }

    // --- Top cap ---
    unsigned int topCenterIndex = data.size() / 8;
    pushVertex(data, { 0, halfH, 0 }, { 0, 1, 0 }, { 0.5f, 0.5f });

    for (int i = 0; i <= segments; ++i)
    {
        float theta = (i / (float)segments) * 2.0f * std::numbers::pi_v<float>;
        float x = cos(theta), z = sin(theta);
        pushVertex(data, { radius * x, halfH, radius * z }, { 0, 1, 0 }, { (x + 1) * 0.5f, (z + 1) * 0.5f });
    }

    for (int i = 0; i < segments; ++i)
    {
        indices.insert(indices.end(), {
            topCenterIndex,
            topCenterIndex + i + 2,
            topCenterIndex + i + 1
            });
    }

    return Mesh::CreateFromIndexedData(
        data.data(), data.size() * sizeof(float),
        indices.data(), indices.size() * sizeof(unsigned int),
        indices.size()
    );
}

// ------------------------------------------------------------
// CONE
// ------------------------------------------------------------
Mesh GeometryFactory::CreateCone(float radius, float height, int segments)
{
    std::vector<float> data;
    std::vector<unsigned int> indices;
    float halfH = height * 0.5f;

    glm::vec3 apex{ 0, halfH, 0 };

    // --- Base ring ---
    for (int i = 0; i <= segments; ++i)
    {
        float theta = (i / (float)segments) * 2.0f * std::numbers::pi_v<float>;
        float x = cos(theta), z = sin(theta);
        glm::vec3 pos{ radius * x, -halfH, radius * z };
        glm::vec3 normal = glm::normalize(glm::vec3(x, radius / height, z));
        pushVertex(data, pos, normal, { (float)i / segments, 0 });
    }

    // --- Apex vertex ---
    pushVertex(data, apex, { 0,1,0 }, { 0.5f,1 });
    unsigned int apexIndex = data.size() / 8 - 1;

    // --- Side triangles ---
    for (int i = 0; i < segments; ++i)
    {
        indices.insert(indices.end(), { (unsigned)i, (unsigned)(i + 1), apexIndex });
    }

    // --- Base center ---
    unsigned int baseCenterIndex = data.size() / 8;
    pushVertex(data, { 0, -halfH, 0 }, { 0, -1, 0 }, { 0.5f, 0.5f });

    // --- Base ring again (for separate normal) ---
    unsigned int baseStart = data.size() / 8;
    for (int i = 0; i <= segments; ++i)
    {
        float theta = (i / (float)segments) * 2.0f * std::numbers::pi_v<float>;
        float x = cos(theta), z = sin(theta);
        pushVertex(data, { radius * x, -halfH, radius * z }, { 0, -1, 0 }, { (x + 1) * 0.5f, (z + 1) * 0.5f });
    }

    // --- Base triangles ---
    for (int i = 0; i < segments; ++i)
    {
        indices.insert(indices.end(), {
            baseCenterIndex,
            baseStart + i + 1,
            baseStart + i
            });
    }

    return Mesh::CreateFromIndexedData(
        data.data(), data.size() * sizeof(float),
        indices.data(), indices.size() * sizeof(unsigned int),
        indices.size()
    );
}

// ------------------------------------------------------------
// TORUS
// ------------------------------------------------------------
Mesh GeometryFactory::CreateTorus(float radius, float tubeRadius, int segments, int rings)
{
    std::vector<float> data;
    std::vector<unsigned int> indices;

    for (int ring = 0; ring <= rings; ++ring)
    {
        float v = (float)ring / rings * 2.0f * std::numbers::pi_v<float>;
        float cosV = cos(v), sinV = sin(v);

        for (int seg = 0; seg <= segments; ++seg)
        {
            float u = (float)seg / segments * 2.0f * std::numbers::pi_v<float>;
            float cosU = cos(u), sinU = sin(u);

            glm::vec3 pos{
                (radius + tubeRadius * cosV) * cosU,
                tubeRadius * sinV,
                (radius + tubeRadius * cosV) * sinU
            };
            glm::vec3 n{
                cosU * cosV,
                sinV,
                sinU * cosV
            };
            pushVertex(data, pos, glm::normalize(n), { (float)seg / segments, (float)ring / rings });
        }
    }

    for (int ring = 0; ring < rings; ++ring)
    {
        for (int seg = 0; seg < segments; ++seg)
        {
            unsigned int i0 = ring * (segments + 1) + seg;
            unsigned int i1 = i0 + segments + 1;
            indices.insert(indices.end(), { i0, i1, i0 + 1, i1, i1 + 1, i0 + 1 });
        }
    }

    return Mesh::CreateFromIndexedData(
        data.data(), data.size() * sizeof(float),
        indices.data(), indices.size() * sizeof(unsigned int),
        indices.size()
    );
}