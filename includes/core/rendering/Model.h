#pragma once
#include <string>
#include <vector>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "helpers/Shader.h"
#include "core/rendering/Mesh.h"
#include "core/rendering/Material.h"
#include "core/rendering/Texture.h"

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
	std::vector<MeshEntry> GetMeshes() const { return meshes; }

	// Helper to setup instancing for all sub-meshes
    void SetupInstancing(const std::vector<glm::mat4>& matrices);
	void Draw(Shader& shader);
	
private:
	// model data
	std::vector<MeshEntry> meshes;
	std::string directory;
	void loadModel(std::string path);
	std::shared_ptr<Texture> LoadStandardMap(TextureType type);
	std::shared_ptr<Texture> LoadMaterialTexture(aiMaterial* mat, aiTextureType type, TextureType typeName);
	void processNode(aiNode* node, const aiScene* scene);
	MeshEntry processMesh(aiMesh* mesh, const aiScene* scene);
	std::vector<std::shared_ptr<Texture>> loadMaterialTextures(aiMaterial* mat,
		aiTextureType type, TextureType typeName);
};
