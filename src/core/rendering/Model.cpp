#include <iostream>
#include <filesystem>
#include <algorithm>
#include <vector>
#include "core/rendering/Model.h"
#include "helpers/Shader.h"
#include "core/ResourceManager.h"
#include "core/rendering/Texture.h"
#include "core/rendering/Material.h"

// Helper to sanitize paths (convert \ to /)
std::string SanitizePath(std::string path) {
    std::replace(path.begin(), path.end(), '\\', '/');
    return path;
}

void Model::Draw(Shader& shader)
{
    for (unsigned int i = 0; i < meshes.size(); i++)
        meshes[i].mesh->Draw(shader, *meshes[i].material);
}

void Model::SetupInstancing(const std::vector<glm::mat4>& matrices)
{
    for (auto& entry : meshes)
    {
        entry.mesh->SetupInstancing(matrices);
    }
}

void Model::loadModel(std::string path)
{
    Assimp::Importer importer;
    // We keep GenNormals and FlipUVs as they are essential for standard rendering
    // added calctangentspace for normal mapping
    const aiScene* scene = importer.ReadFile(path, 
        aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenNormals 
        | aiProcess_CalcTangentSpace);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        std::cout << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
        return;
    }

    directory = std::filesystem::path(path).parent_path().string();
    directory = SanitizePath(directory); // Ensure generic format immediately
    processNode(scene->mRootNode, scene);
}

void Model::processNode(aiNode* node, const aiScene* scene)
{
    for (unsigned int i = 0; i < node->mNumMeshes; i++)
    {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        meshes.push_back(processMesh(mesh, scene));
    }
    for (unsigned int i = 0; i < node->mNumChildren; i++)
    {
        processNode(node->mChildren[i], scene);
    }
}

// helper to load texture from assimp material
std::shared_ptr<Texture> Model::LoadMaterialTexture(aiMaterial* mat, aiTextureType type, TextureType typeName)
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
    if (type == TextureType::TEX_DIFFUSE) {
        standardNames = { "diffuse.png", "diffuse.jpg", "albedo.png", "albedo.jpg", "base.png", "base.jpg" };
    }
    else if (type == TextureType::TEX_SPECULAR) {
        standardNames = { "specular.png", "specular.jpg", "spec.png", "spec.jpg", "roughness.png", "roughness.jpg" };
    }
    // added normal map standard names
    else if (type == TextureType::TEX_NORMAL) {
        standardNames = { "normal.png", "normal.jpg", "norm.png", "norm.jpg" };
    }

    // Scan the directory
    for (const auto& name : standardNames)
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

MeshEntry Model::processMesh(aiMesh* mesh, const aiScene* scene)
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
        if (mesh->HasNormals()) {
            vector.x = mesh->mNormals[i].x;
            vector.y = mesh->mNormals[i].y;
            vector.z = mesh->mNormals[i].z;
            vertex.Normal = vector;
        } else {
            vertex.Normal = glm::vec3(0.0f, 1.0f, 0.0f);
        }

        // Texture Coords
        if (mesh->mTextureCoords[0]) {
            glm::vec2 vec;
            vec.x = mesh->mTextureCoords[0][i].x;
            vec.y = mesh->mTextureCoords[0][i].y;
            vertex.TexCoords = vec;
        } else {
            vertex.TexCoords = glm::vec2(0.0f, 0.0f);
        }

        // tangent calculation
        if (mesh->HasTangentsAndBitangents()) {
            vector.x = mesh->mTangents[i].x;
            vector.y = mesh->mTangents[i].y;
            vector.z = mesh->mTangents[i].z;
            vertex.Tangent = vector;
        } else {
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

    // Process Material (Engine Standard Way)
    Material mat;
    
    // Default Material Settings
    mat.defaultDiffuseColor = glm::vec3(1.0f, 1.0f, 1.0f);
    mat.defaultShininess = 32.0f;

    // hybrid approach: try assimp path first, fallback to directory scan
    if (mesh->mMaterialIndex >= 0)
    {
        aiMaterial* aMat = scene->mMaterials[mesh->mMaterialIndex];

        // diffuse map
        auto diffuseTex = LoadMaterialTexture(aMat, aiTextureType_DIFFUSE, TextureType::TEX_DIFFUSE);
        if (!diffuseTex) diffuseTex = LoadMaterialTexture(aMat, aiTextureType_BASE_COLOR, TextureType::TEX_DIFFUSE);
        if (!diffuseTex) diffuseTex = LoadStandardMap(TextureType::TEX_DIFFUSE);
        if (diffuseTex) mat.textures["material_diffuse"] = diffuseTex;

        // specular map
        auto specTex = LoadMaterialTexture(aMat, aiTextureType_SPECULAR, TextureType::TEX_SPECULAR);
        if (!specTex) specTex = LoadStandardMap(TextureType::TEX_SPECULAR);
        if (!specTex) specTex = LoadMaterialTexture(aMat, aiTextureType_METALNESS, TextureType::TEX_SPECULAR);
        if (specTex) mat.textures["material_specular"] = specTex;

        // normal map (check normals and height slots)
        auto normTex = LoadMaterialTexture(aMat, aiTextureType_NORMALS, TextureType::TEX_NORMAL);
        if (!normTex) normTex = LoadMaterialTexture(aMat, aiTextureType_HEIGHT, TextureType::TEX_NORMAL);
        if (!normTex) normTex = LoadStandardMap(TextureType::TEX_NORMAL);
        if (normTex) mat.textures["material_normal"] = normTex;

        // B. Load Fallback Properties from Assimp (Colors/Shininess)
        // We still read these just in case the model has specific color data we want to respect
        aiColor3D color(1.0f, 1.0f, 1.0f);

        if (AI_SUCCESS == aMat->Get(AI_MATKEY_COLOR_DIFFUSE, color)) {
            mat.vec3s["material_diffuseColor"] = glm::vec3(color.r, color.g, color.b);
        }
        if (AI_SUCCESS == aMat->Get(AI_MATKEY_COLOR_SPECULAR, color)) {
            mat.vec3s["material_specularColor"] = glm::vec3(color.r, color.g, color.b);
        }

        float shininess = 0.0f;
        if (AI_SUCCESS == aMat->Get(AI_MATKEY_SHININESS, shininess)) {
            mat.floats["material_shininess"] = shininess;
        }
    }

    MeshEntry entry;
    entry.mesh = std::make_shared<Mesh>(vertices, indices);
    entry.material = std::make_shared<Material>(mat);
    return entry;
}