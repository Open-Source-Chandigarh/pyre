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
    std::vector<Texture> textures;

    // ---- Vertices ----
    for (unsigned int i = 0; i < mesh->mNumVertices; i++)
    {
        Vertex vertex;
        glm::vec3 vector;

        vector.x = mesh->mVertices[i].x;
        vector.y = mesh->mVertices[i].y;
        vector.z = mesh->mVertices[i].z;
        vertex.Position = vector;

        vector.x = mesh->mNormals[i].x;
        vector.y = mesh->mNormals[i].y;
        vector.z = mesh->mNormals[i].z;
        vertex.Normal = vector;

        if (mesh->mTextureCoords[0])
        {
            glm::vec2 vec;
            vec.x = mesh->mTextureCoords[0][i].x;
            vec.y = mesh->mTextureCoords[0][i].y;
            vertex.TexCoords = vec;
        }
        else
            vertex.TexCoords = glm::vec2(0.0f);

        vertices.push_back(vertex);
    }

    // ---- Indices ----
    for (unsigned int i = 0; i < mesh->mNumFaces; i++)
    {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++)
            indices.push_back(face.mIndices[j]);
    }

    // ---- Material ----
    Material mat;
    mat.diffuseColor = glm::vec3(1.0f, 1.0f, 1.0f);
    mat.specularColor = glm::vec3(1.0f, 1.0f, 1.0f);
    mat.shininess = 32.0f;
    mat.useDiffuseMap = false;
    mat.useSpecularMap = false;

    if (mesh->mMaterialIndex >= 0)
    {
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

        // Diffuse
        std::vector<std::shared_ptr<Texture>> diffuseMaps =
            loadMaterialTextures(material, aiTextureType_DIFFUSE, TextureType::TEX_DIFFUSE);
        if (!diffuseMaps.empty())
        {
            mat.textures.insert(mat.textures.end(), diffuseMaps.begin(), diffuseMaps.end());
            mat.useDiffuseMap = true;
        }

        // Specular
        std::vector<std::shared_ptr<Texture>> specularMaps =
            loadMaterialTextures(material, aiTextureType_SPECULAR, TextureType::TEX_SPECULAR);
        if (!specularMaps.empty())
        {
            mat.textures.insert(mat.textures.end(), specularMaps.begin(), specularMaps.end());
            mat.useSpecularMap = true;
        }

        // Optional: get base colors from material if no textures
        aiColor3D color(1.0f, 1.0f, 1.0f);
        if (AI_SUCCESS == material->Get(AI_MATKEY_COLOR_DIFFUSE, color))
            mat.diffuseColor = glm::vec3(color.r, color.g, color.b);
        if (AI_SUCCESS == material->Get(AI_MATKEY_COLOR_SPECULAR, color))
            mat.specularColor = glm::vec3(color.r, color.g, color.b);

        float shininess = 0.0f;
        if (AI_SUCCESS == material->Get(AI_MATKEY_SHININESS, shininess))
            mat.shininess = shininess;
    }

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
		std::shared_ptr<Texture> texture;
		std::string fileName = std::string(str.C_Str());
		texture = ResourceManager::LoadTexture(
					this->directory + '/' + fileName, typeName);
		textures.push_back(texture);
	}
	return textures;
}