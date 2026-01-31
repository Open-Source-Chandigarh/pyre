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

struct ModelNode {
    std::shared_ptr<Mesh> mesh;
    glm::mat4 localTransform = glm::mat4(1.0f);
};

class Model
{
public:
	// model data
	std::vector<ModelNode> nodes;
	Model(const std::string& path)
	{
		loadModel(path);
	}
	size_t GetNodeCount() const { return nodes.size(); }

	void SetupInstancing(const std::vector<glm::mat4> &matrices) 
	{
		for (auto& node : nodes)
    	{
        	if (node.mesh)
        	{
            	// Forward the matrices to the Mesh's VBO setup
            	node.mesh->SetupInstancing(matrices);
        	}
    	}
	};
	
private:
	std::string directory;
	void loadModel(std::string path);
	std::shared_ptr<Texture> LoadStandardMap(TextureType type);
	std::shared_ptr<Texture> LoadMaterialTexture(aiMaterial* mat, aiTextureType type, TextureType typeName);
	void processNode(aiNode* node, const aiScene* scene);
	ModelNode processMesh(aiMesh* mesh, const aiScene* scene);
};
