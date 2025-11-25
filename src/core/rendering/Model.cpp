#include <iostream>
#include <filesystem>
#include "core/rendering/Model.h"
#include "helpers/shaderClass.h"
#include "core/ResourceManager.h"

void Model::Draw(Shader& shader)
{
	for (unsigned int i = 0; i < meshes.size(); i++)
		meshes[i].mesh -> Draw(shader, *meshes[i].material);
}

void Model::loadModel(std::string path)
{
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate |
		aiProcess_FlipUVs);

	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE ||
		!scene->mRootNode)
	{
		std::cout << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
		return;
	}

	directory = std::filesystem::path(path).parent_path().string();
	processNode(scene->mRootNode, scene);
}

void Model::processNode(aiNode* node, const aiScene* scene)
{
	// process all the node’s meshes (if any)
	for (unsigned int i = 0; i < node->mNumMeshes; i++)
	{
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
		meshes.push_back(processMesh(mesh, scene));
	}
	// then do the same for each of its children
	for (unsigned int i = 0; i < node->mNumChildren; i++)
	{
		processNode(node->mChildren[i], scene);
	}
}

MeshEntry Model::processMesh(aiMesh* mesh, const aiScene* scene)
{
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    // ---- Vertices ----
    for (unsigned int i = 0; i < mesh->mNumVertices; i++)
    {
        Vertex vertex;
        glm::vec3 vector;

        vector.x = mesh->mVertices[i].x;
        vector.y = mesh->mVertices[i].y;
        vector.z = mesh->mVertices[i].z;
        vertex.Position = vector;

        // normals (guard if missing)
        if (mesh->HasNormals()) {
            vector.x = mesh->mNormals[i].x;
            vector.y = mesh->mNormals[i].y;
            vector.z = mesh->mNormals[i].z;
            vertex.Normal = vector;
        }
        else {
            vertex.Normal = glm::vec3(0.0f, 1.0f, 0.0f);
        }

        // texcoords (guard if missing)
        if (mesh->mTextureCoords[0]) {
            glm::vec2 vec;
            vec.x = mesh->mTextureCoords[0][i].x;
            vec.y = mesh->mTextureCoords[0][i].y;
            vertex.TexCoords = vec;
        }
        else {
            vertex.TexCoords = glm::vec2(0.0f, 0.0f);
        }

        vertices.push_back(vertex);
    }

    // ---- Indices ----
    for (unsigned int i = 0; i < mesh->mNumFaces; i++)
    {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++)
            indices.push_back(face.mIndices[j]);
    }

    // ---- Material (NEW API) ----
    Material mat; // uses unordered_map textures/floats/vec3s and default fields

    // set default fallback colors/shininess (will be used if shader doesn't provide textures)
    mat.defaultDiffuseColor = glm::vec3(1.0f, 1.0f, 1.0f);
    mat.defaultShininess = 32.0f;

    if (mesh->mMaterialIndex >= 0)
    {
        aiMaterial* aMat = scene->mMaterials[mesh->mMaterialIndex];

        // Load diffuse textures (take first if multiple)
        std::vector<std::shared_ptr<Texture>> diffuseMaps =
            loadMaterialTextures(aMat, aiTextureType_DIFFUSE, TextureType::TEX_DIFFUSE);
        if (!diffuseMaps.empty() && diffuseMaps[0]) {
            // key must match shader/sample name in material_common.glsl
            mat.textures["material_diffuse"] = diffuseMaps[0];
        }

        // Load specular textures (take first)
        std::vector<std::shared_ptr<Texture>> specMaps =
            loadMaterialTextures(aMat, aiTextureType_SPECULAR, TextureType::TEX_SPECULAR);
        if (!specMaps.empty() && specMaps[0]) {
            mat.textures["material_specular"] = specMaps[0];
        }

        // Optional: get base colors from aiMaterial and set vec3s fallback
        aiColor3D color(1.0f, 1.0f, 1.0f);
        if (AI_SUCCESS == aMat->Get(AI_MATKEY_COLOR_DIFFUSE, color)) {
            mat.vec3s["material_diffuseColor"] = glm::vec3(color.r, color.g, color.b);
            mat.defaultDiffuseColor = glm::vec3(color.r, color.g, color.b);
        }
        if (AI_SUCCESS == aMat->Get(AI_MATKEY_COLOR_SPECULAR, color)) {
            mat.vec3s["material_specularColor"] = glm::vec3(color.r, color.g, color.b);
            mat.defaultDiffuseColor = mat.defaultDiffuseColor; // keep existing
        }

        float shininess = 0.0f;
        if (AI_SUCCESS == aMat->Get(AI_MATKEY_SHININESS, shininess)) {
            mat.floats["material_shininess"] = shininess;
            mat.defaultShininess = shininess;
        }
    }

    // Build mesh entry
    MeshEntry entry;
    entry.mesh = std::make_shared<Mesh>(vertices, indices);
    entry.material = std::make_shared<Material>(mat);
    return entry;
}

std::vector<std::shared_ptr<Texture>> Model::loadMaterialTextures(aiMaterial* mat,
    aiTextureType type, TextureType typeName)
{
    std::vector<std::shared_ptr<Texture>> textures;

    for (unsigned int i = 0; i < mat->GetTextureCount(type); i++)
    {
        aiString str;
        mat->GetTexture(type, i, &str);
        std::string fileName = std::string(str.C_Str());
        auto texture = ResourceManager::LoadTexture(this->directory + '/' + fileName, typeName);
        if (texture) textures.push_back(texture);
    }
    return textures;
}