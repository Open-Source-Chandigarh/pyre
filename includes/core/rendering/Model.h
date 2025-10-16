#pragma once
#include <string>
#include <vector>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "helpers/shaderClass.h";
#include "core/rendering/Mesh.h"

struct MeshEntry {
	std::shared_ptr<Mesh> mesh;
	std::shared_ptr<Material> material;
};

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
	std::vector<MeshEntry> meshes;
	std::string directory;
	void loadModel(std::string path);
	void processNode(aiNode* node, const aiScene* scene);
	MeshEntry processMesh(aiMesh* mesh, const aiScene* scene);
	std::vector<std::shared_ptr<Texture>> loadMaterialTextures(aiMaterial* mat,
		aiTextureType type, TextureType typeName);
};
