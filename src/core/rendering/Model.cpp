#include "core/rendering/Model.h"
#include "core/ResourceManager.h"
#include "core/rendering/Material.h"
#include "core/rendering/Texture.h"
#include "helpers/Shader.h"
#include <algorithm>
#include <filesystem>
#include <iostream>
#include <vector>

// Helper to sanitize paths (convert \ to /)
std::string SanitizePath(std::string path)
{
    std::replace(path.begin(), path.end(), '\\', '/');
    return path;
}

inline glm::mat4 AssimpToGLM(const aiMatrix4x4 &from)
{
    glm::mat4 to;
    // assimp is row-major, glm is column-major we transpose it during assignment
    to[0][0] = from.a1;
    to[1][0] = from.a2;
    to[2][0] = from.a3;
    to[3][0] = from.a4;
    to[0][1] = from.b1;
    to[1][1] = from.b2;
    to[2][1] = from.b3;
    to[3][1] = from.b4;
    to[0][2] = from.c1;
    to[1][2] = from.c2;
    to[2][2] = from.c3;
    to[3][2] = from.c4;
    to[0][3] = from.d1;
    to[1][3] = from.d2;
    to[2][3] = from.d3;
    to[3][3] = from.d4;
    return to;
}

void Model::loadModel(std::string path)
{
    Assimp::Importer importer;
    unsigned int importFlags = aiProcess_Triangulate | aiProcess_GenNormals | aiProcess_CalcTangentSpace;
    
    std::string ext = std::filesystem::path(path).extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    
    // conditionally add FlipUVs for formats like .obj and .fbx
    // glTF natively defines UVs differently, so applying FlipUVs to them causes a double flip
    if (ext != ".gltf" && ext != ".glb")
    {
        importFlags |= aiProcess_FlipUVs;
    }
    
    const aiScene *scene = importer.ReadFile(path, importFlags);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        std::cout << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
        return;
    }

    directory = std::filesystem::path(path).parent_path().string();
    directory = SanitizePath(directory); // Ensure generic format immediately
    processNode(scene->mRootNode, scene, glm::mat4(1.0f));
}

void Model::processNode(aiNode *node, const aiScene *scene, glm::mat4 parentTransform)
{
    glm::mat4 nodeTransform = AssimpToGLM(node->mTransformation);
    glm::mat4 globalTransform = parentTransform * nodeTransform;

    for (unsigned int i = 0; i < node->mNumMeshes; i++)
    {
        aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
        nodes.push_back(processMesh(mesh, scene, globalTransform));
    }
    for (unsigned int i = 0; i < node->mNumChildren; i++)
    {
        processNode(node->mChildren[i], scene, globalTransform);
    }
}

// helper to load texture from assimp material
std::shared_ptr<Texture> Model::LoadMaterialTexture(aiMaterial *mat, aiTextureType type, TextureType typeName)
{
    if (mat->GetTextureCount(type) > 0)
    {
        aiString str;
        mat->GetTexture(type, 0, &str);

        // Use std::filesystem to extract just the filename (e.g. "diffuse.png")
        // avoiding issues with absolute paths baked into FBX files
        std::string filename = std::filesystem::path(str.C_Str()).filename().string();

        std::string fullPath = this->directory + "/" + filename;
        fullPath = SanitizePath(fullPath);

        if (std::filesystem::exists(fullPath))
        {
            return ResourceManager::LoadTexture(fullPath, typeName);
        }
        else
        {
            std::cout << "[Model] Missing Texture: " << fullPath << std::endl;
        }
    }
    return nullptr;
}

// We ignore what the .fbx/.obj says internally. We only look for specific filenames.
std::shared_ptr<Texture> Model::LoadStandardMap(TextureType type)
{
    std::vector<std::string> standardNames;

    // Define our engine standards here
    if (type == TextureType::TEX_DIFFUSE)
    {
        standardNames = {"diffuse.png", "diffuse.jpg", "albedo.png", "albedo.jpg", "base.png", "base.jpg"};
    }
    else if (type == TextureType::TEX_SPECULAR)
    {
        standardNames = {"specular.png", "specular.jpg", "spec.png", "spec.jpg"};
    }
    // added normal map standard names
    else if (type == TextureType::TEX_NORMAL)
    {
        standardNames = {"normal.png", "normal.jpg", "norm.png", "norm.jpg"};
    }
    else if (type == TextureType::TEX_METALLIC)
    {
        standardNames = {"metallic.png", "metallic.jpg", "metalness.png", "metalness.jpg", "metal.png", "metal.jpg"};
    }
    else if (type == TextureType::TEX_ROUGHNESS)
    {
        standardNames = {"roughness.png", "roughness.jpg", "rough.png", "rough.jpg"};
    }
    else if (type == TextureType::TEX_DISPLACEMENT)
    {
        standardNames = {"displacement.png", "displacement.jpg", "disp.png", "disp.jpg", "height.png", "height.jpg"};
    }

    // Scan the directory
    for (const auto &name : standardNames)
    {
        std::string fullPath = this->directory + "/" + name;
        if (std::filesystem::exists(fullPath))
        {
            std::cout << "[Model] Auto-detected standard map: " << fullPath << "\n";
            return ResourceManager::LoadTexture(fullPath, type);
        }
    }

    return nullptr; // No standard map found
}

ModelNode Model::processMesh(aiMesh *mesh, const aiScene *scene, glm::mat4 transform)
{
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    // Process Geometry
    for (unsigned int i = 0; i < mesh->mNumVertices; i++)
    {
        Vertex vertex;
        glm::vec3 vector;

        // Position
        vector.x = mesh->mVertices[i].x;
        vector.y = mesh->mVertices[i].y;
        vector.z = mesh->mVertices[i].z;
        vertex.Position = vector;

        // Normal
        if (mesh->HasNormals())
        {
            vector.x = mesh->mNormals[i].x;
            vector.y = mesh->mNormals[i].y;
            vector.z = mesh->mNormals[i].z;
            vertex.Normal = vector;
        }
        else
        {
            vertex.Normal = glm::vec3(0.0f, 1.0f, 0.0f);
        }

        // Texture Coords
        if (mesh->mTextureCoords[0])
        {
            glm::vec2 vec;
            vec.x = mesh->mTextureCoords[0][i].x;
            vec.y = mesh->mTextureCoords[0][i].y;
            vertex.TexCoords = vec;
        }
        else
        {
            vertex.TexCoords = glm::vec2(0.0f, 0.0f);
        }

        // tangent calculation
        if (mesh->HasTangentsAndBitangents())
        {
            vector.x = mesh->mTangents[i].x;
            vector.y = mesh->mTangents[i].y;
            vector.z = mesh->mTangents[i].z;
            vertex.Tangent = vector;
        }
        else
        {
            vertex.Tangent = glm::vec3(0.0f);
        }

        vertices.push_back(vertex);
    }

    for (unsigned int i = 0; i < mesh->mNumFaces; i++)
    {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++)
            indices.push_back(face.mIndices[j]);
    }

    // Process Material (Base Material)
    // We create a new Material instance for every mesh to store its Assimp properties
    std::shared_ptr<Material> baseMat = std::make_shared<Material>();

    // hybrid approach: try assimp path first, fallback to directory scan
    if (mesh->mMaterialIndex >= 0)
    {
        aiMaterial *aMat = scene->mMaterials[mesh->mMaterialIndex];

        // diffuse map
        auto diffuseTex = LoadMaterialTexture(aMat, aiTextureType_DIFFUSE, TextureType::TEX_DIFFUSE);
        if (!diffuseTex)
            diffuseTex = LoadMaterialTexture(aMat, aiTextureType_BASE_COLOR, TextureType::TEX_DIFFUSE);
        if (!diffuseTex)
            diffuseTex = LoadStandardMap(TextureType::TEX_DIFFUSE);
        if (diffuseTex)
            baseMat->textures["material_diffuse"] = diffuseTex;

        // specular map
        auto specTex = LoadMaterialTexture(aMat, aiTextureType_SPECULAR, TextureType::TEX_SPECULAR);
        if (!specTex)
            specTex = LoadStandardMap(TextureType::TEX_SPECULAR);
        if (specTex)
            baseMat->textures["material_specular"] = specTex;

        // normal map (check normals and height slots)
        auto normTex = LoadMaterialTexture(aMat, aiTextureType_NORMALS, TextureType::TEX_NORMAL);
        if (!normTex)
            normTex = LoadMaterialTexture(aMat, aiTextureType_HEIGHT, TextureType::TEX_NORMAL);
        if (!normTex)
            normTex = LoadStandardMap(TextureType::TEX_NORMAL);
        if (normTex)
            baseMat->textures["material_normal"] = normTex;

        auto dispTex = LoadMaterialTexture(aMat, aiTextureType_DISPLACEMENT, TextureType::TEX_DISPLACEMENT);
        if (!dispTex)
            dispTex = LoadStandardMap(TextureType::TEX_DISPLACEMENT);
        if (dispTex)
            baseMat->textures["material_displacement"] = dispTex;

        // PBR metallic map
        auto metalTex = LoadMaterialTexture(aMat, aiTextureType_METALNESS, TextureType::TEX_METALLIC);
        if (!metalTex)
            metalTex = LoadStandardMap(TextureType::TEX_METALLIC);
        if (metalTex)
            baseMat->textures["material_metallic"] = metalTex;

        // PBR roughness map
        auto roughTex = LoadMaterialTexture(aMat, aiTextureType_DIFFUSE_ROUGHNESS, TextureType::TEX_ROUGHNESS);
        if (!roughTex)
            roughTex = LoadStandardMap(TextureType::TEX_ROUGHNESS);
        if (roughTex)
            baseMat->textures["material_roughness"] = roughTex;

        // B. Load Fallback Properties from Assimp (Colors/Shininess)
        // We still read these just in case the model has specific color data we want to respect
        aiColor3D color(1.0f, 1.0f, 1.0f);

        if (AI_SUCCESS == aMat->Get(AI_MATKEY_COLOR_DIFFUSE, color))
        {
            baseMat->vec3s["material_diffuseColor"] = glm::vec3(color.r, color.g, color.b);
        }
        if (AI_SUCCESS == aMat->Get(AI_MATKEY_COLOR_SPECULAR, color))
        {
            baseMat->vec3s["material_specularColor"] = glm::vec3(color.r, color.g, color.b);
        }

        float shininess = 0.0f;
        if (AI_SUCCESS == aMat->Get(AI_MATKEY_SHININESS, shininess))
        {
            baseMat->floats["material_shininess"] = shininess;
        }
        else
        {
            baseMat->floats["material_shininess"] = 32.0f; // Default
        }
    }

    ModelNode node;
    // Pass the baseMat into the Mesh constructor
    node.mesh = std::make_shared<Mesh>(vertices, indices, baseMat);
    node.baseMaterial = baseMat;
    node.localTransform = transform;
    return node;
}