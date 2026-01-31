#include "core/rendering/geometry/GeometryFactory.h"
#include <array>
#include <numbers>
#include <cmath>
#include <glm/glm.hpp>

using namespace GeometryFactory;

// Pushes 11 floats (Pos, Normal, UV, Tangent)
static inline void PushVertex(std::vector<float>& v, glm::vec3 pos, glm::vec3 n, glm::vec2 uv, glm::vec3 t)
{
    v.insert(v.end(), { 
        pos.x, pos.y, pos.z, 
        n.x, n.y, n.z, 
        uv.x, uv.y,
        t.x, t.y, t.z
    });
}

// Triangle-based Tangent Calculation
static glm::vec3 CalcTangent(
    glm::vec3 p1, glm::vec3 p2, glm::vec3 p3, 
    glm::vec2 uv1, glm::vec2 uv2, glm::vec2 uv3)
{
    glm::vec3 edge1 = p2 - p1;
    glm::vec3 edge2 = p3 - p1;
    glm::vec2 deltaUV1 = uv2 - uv1;
    glm::vec2 deltaUV2 = uv3 - uv1;

    float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

    glm::vec3 tangent;
    tangent.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
    tangent.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
    tangent.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);
    
    return glm::normalize(tangent);
}

// Converts raw float buffer (stride 11) to Mesh object
static std::shared_ptr<Mesh> BuildMeshFromData(const std::vector<float>& data, const std::vector<unsigned int>& indices)
{
    std::vector<Vertex> vertices;
    vertices.resize(data.size() / 11);

    for(size_t i = 0; i < vertices.size(); i++)
    {
        size_t base = i * 11;
        vertices[i].Position  = glm::vec3(data[base+0], data[base+1], data[base+2]);
        vertices[i].Normal    = glm::vec3(data[base+3], data[base+4], data[base+5]);
        vertices[i].TexCoords = glm::vec2(data[base+6], data[base+7]);
        vertices[i].Tangent   = glm::vec3(data[base+8], data[base+9], data[base+10]);
    }

    // Single allocation for both Control Block and Mesh
    return std::make_shared<Mesh>(vertices, indices, nullptr);
}

std::shared_ptr<Mesh> GeometryFactory::CreateCube(float size)
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

    auto quad = [&](int a, int b, int c, int d, glm::vec3 n)
    {
        // Calculate tangent based on the first triangle
        glm::vec3 t = CalcTangent(positions[a], positions[b], positions[c], uvs[0], uvs[1], uvs[2]);

        unsigned int startIndex = data.size() / 11; 

        PushVertex(data, positions[a], n, uvs[0], t);
        PushVertex(data, positions[b], n, uvs[1], t);
        PushVertex(data, positions[c], n, uvs[2], t);
        PushVertex(data, positions[d], n, uvs[3], t);

        indices.insert(indices.end(), { 
            startIndex, startIndex + 1, startIndex + 2,
            startIndex, startIndex + 2, startIndex + 3 
        });
    };

    quad(4, 5, 6, 7, normals[1]); // front
    quad(1, 0, 3, 2, normals[0]); // back
    quad(5, 1, 2, 6, normals[2]); // right
    quad(0, 4, 7, 3, normals[3]); // left
    quad(3, 7, 6, 2, normals[4]); // top
    quad(0, 1, 5, 4, normals[5]); // bottom

    return BuildMeshFromData(data, indices);
}

std::shared_ptr<Mesh> GeometryFactory::CreateSkyboxCube(float size)
{
    // Skybox does not need tangents or lighting, keeping it simple (Positions Only)
    float h = size * 0.5f;
    static const float skyboxVertices[] = {
        -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

        -1.0f,  1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f,  1.0f
    };

    std::vector<float> vBuffer(std::begin(skyboxVertices), std::end(skyboxVertices));

    if (size != 1.0f) {
        for (float& f : vBuffer) f *= h;
    }

    Mesh temp = Mesh::CreatePositionsOnly(vBuffer.data(), vBuffer.size() * sizeof(float), 36);
    return std::make_shared<Mesh>(std::move(temp));
}

std::shared_ptr<Mesh> GeometryFactory::CreatePlane(float size)
{
    float h = size * 0.5f;
    std::vector<float> data;
    std::vector<unsigned int> indices = { 0,1,2, 0,2,3 };

    glm::vec3 p1 = { -h,0,-h };
    glm::vec3 p2 = { h,0,-h };
    glm::vec3 p3 = { h,0, h };
    glm::vec3 p4 = { -h,0, h };
    glm::vec3 n = { 0,1,0 };

    glm::vec2 uv1 = { 0,0 };
    glm::vec2 uv2 = { 1,0 };
    glm::vec2 uv3 = { 1,1 };
    glm::vec2 uv4 = { 0,1 };

    glm::vec3 t = CalcTangent(p1, p2, p3, uv1, uv2, uv3);

    PushVertex(data, p1, n, uv1, t);
    PushVertex(data, p2, n, uv2, t);
    PushVertex(data, p3, n, uv3, t);
    PushVertex(data, p4, n, uv4, t);

    return BuildMeshFromData(data, indices);
}

std::shared_ptr<Mesh> GeometryFactory::CreateSphere(float radius, int segments, int rings)
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
            float theta = u * std::numbers::pi_v<float> * 2.0f;

            glm::vec3 pos{
                radius * sin(phi) * cos(theta),
                radius * cos(phi),
                radius * sin(phi) * sin(theta)
            };
            glm::vec3 n = glm::normalize(pos);

            // Analytical Tangent for Sphere (follows U/Longitude)
            // Derivative of Pos w.r.t theta is (-sin(theta), 0, cos(theta)) 
            // We ignore Y component because it is driven by Phi, not Theta.
            glm::vec3 t = glm::normalize(glm::vec3(
                -sin(theta), 
                0.0f, 
                cos(theta)
            ));

            PushVertex(data, pos, n, { u, v }, t);
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

    return BuildMeshFromData(data, indices);
}

std::shared_ptr<Mesh> GeometryFactory::CreateCylinder(float radius, float height, int segments)
{
    std::vector<float> data;
    std::vector<unsigned int> indices;
    float halfH = height * 0.5f;

    // Side Vertices
    for (int i = 0; i <= segments; ++i)
    {
        float theta = (i / (float)segments) * 2.0f * std::numbers::pi_v<float>;
        float x = cos(theta), z = sin(theta);
        glm::vec3 normal{ x, 0, z };
        
        // Tangent is horizontal along the circle
        glm::vec3 tangent{ -sin(theta), 0.0f, cos(theta) };

        PushVertex(data, { radius * x, -halfH, radius * z }, normal, { (float)i / segments, 0 }, tangent);
        PushVertex(data, { radius * x,  halfH, radius * z }, normal, { (float)i / segments, 1 }, tangent);
    }

    // Side Indices
    for (int i = 0; i < segments; ++i)
    {
        unsigned int base = i * 2;
        indices.insert(indices.end(), { base, base + 1, base + 2, base + 1, base + 3, base + 2 });
    }

    // Bottom Cap
    unsigned int bottomCenterIndex = data.size() / 11;
    // Planar mapping -> Tangent along X
    glm::vec3 capTangent{ 1.0f, 0.0f, 0.0f }; 

    PushVertex(data, { 0, -halfH, 0 }, { 0, -1, 0 }, { 0.5f, 0.5f }, capTangent);

    for (int i = 0; i <= segments; ++i)
    {
        float theta = (i / (float)segments) * 2.0f * std::numbers::pi_v<float>;
        float x = cos(theta), z = sin(theta);
        PushVertex(data, { radius * x, -halfH, radius * z }, { 0, -1, 0 }, { (x + 1) * 0.5f, (z + 1) * 0.5f }, capTangent);
    }

    for (int i = 0; i < segments; ++i)
    {
        indices.insert(indices.end(), {
            bottomCenterIndex,
            bottomCenterIndex + i + 1,
            bottomCenterIndex + i + 2
            });
    }

    // Top Cap
    unsigned int topCenterIndex = data.size() / 11;
    PushVertex(data, { 0, halfH, 0 }, { 0, 1, 0 }, { 0.5f, 0.5f }, capTangent);

    for (int i = 0; i <= segments; ++i)
    {
        float theta = (i / (float)segments) * 2.0f * std::numbers::pi_v<float>;
        float x = cos(theta), z = sin(theta);
        PushVertex(data, { radius * x, halfH, radius * z }, { 0, 1, 0 }, { (x + 1) * 0.5f, (z + 1) * 0.5f }, capTangent);
    }

    for (int i = 0; i < segments; ++i)
    {
        indices.insert(indices.end(), {
            topCenterIndex,
            topCenterIndex + i + 2,
            topCenterIndex + i + 1
            });
    }

    return BuildMeshFromData(data, indices);
}

std::shared_ptr<Mesh> GeometryFactory::CreateCone(float radius, float height, int segments)
{
    std::vector<float> data;
    std::vector<unsigned int> indices;
    float halfH = height * 0.5f;

    glm::vec3 apex{ 0, halfH, 0 };

    // Side Ring
    for (int i = 0; i <= segments; ++i)
    {
        float theta = (i / (float)segments) * 2.0f * std::numbers::pi_v<float>;
        float x = cos(theta), z = sin(theta);
        glm::vec3 pos{ radius * x, -halfH, radius * z };
        
        // Normal is tilted
        glm::vec3 normal = glm::normalize(glm::vec3(x, radius / height, z));
        
        // Tangent is horizontal along the ring
        glm::vec3 tangent{ -sin(theta), 0.0f, cos(theta) };

        PushVertex(data, pos, normal, { (float)i / segments, 0 }, tangent);
    }

    // Apex vertex (Texture coordinate top center)
    // Tangent at the tip is ambiguous, defaulting to X-axis
    PushVertex(data, apex, { 0,1,0 }, { 0.5f, 1 }, { 1.0f, 0.0f, 0.0f });
    
    unsigned int apexIndex = data.size() / 11 - 1;

    // Side triangles
    for (int i = 0; i < segments; ++i)
    {
        indices.insert(indices.end(), { (unsigned)i, (unsigned)(i + 1), apexIndex });
    }

    // Base Center
    unsigned int baseCenterIndex = data.size() / 11;
    glm::vec3 baseTangent{ 1.0f, 0.0f, 0.0f };
    
    PushVertex(data, { 0, -halfH, 0 }, { 0, -1, 0 }, { 0.5f, 0.5f }, baseTangent);

    // Base ring again (for separate normal)
    unsigned int baseStart = data.size() / 11;
    for (int i = 0; i <= segments; ++i)
    {
        float theta = (i / (float)segments) * 2.0f * std::numbers::pi_v<float>;
        float x = cos(theta), z = sin(theta);
        PushVertex(data, { radius * x, -halfH, radius * z }, { 0, -1, 0 }, { (x + 1) * 0.5f, (z + 1) * 0.5f }, baseTangent);
    }

    // Base triangles
    for (int i = 0; i < segments; ++i)
    {
        indices.insert(indices.end(), {
            baseCenterIndex,
            baseStart + i + 1,
            baseStart + i
            });
    }

    return BuildMeshFromData(data, indices);
}

std::shared_ptr<Mesh> GeometryFactory::CreateTorus(float radius, float tubeRadius, int segments, int rings)
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
            
            // Tangent follows the major ring (U direction)
            // Normalized derivative w.r.t U: (-sinU, 0, cosU)
            glm::vec3 t{ -sinU, 0.0f, cosU };

            PushVertex(data, pos, glm::normalize(n), { (float)seg / segments, (float)ring / rings }, glm::normalize(t));
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

    return BuildMeshFromData(data, indices);
}