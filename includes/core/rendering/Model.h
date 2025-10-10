#pragma once
#include <string>
#include <vector>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "helpers/shaderClass.h";
#include "core/rendering/Mesh.h"

class Model
{
public:
	Model(const std::string& path)
	{
		loadModel(path);
	}
	size_t GetMeshCount() const { return meshes.size(); }
	void Draw(Shader& shader);
private:
	// model data
	std::vector<Mesh> meshes;
	std::string directory;
	void loadModel(std::string path);
	void processNode(aiNode* node, const aiScene* scene);
	Mesh processMesh(aiMesh* mesh, const aiScene* scene);
	std::vector<Texture> loadMaterialTextures(aiMaterial* mat,
		aiTextureType type, std::string typeName);
};
