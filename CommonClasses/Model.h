#pragma once


/*
* Do I need all of these?
*/
#include <string>
#include <vector>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "Mesh.h"

//class Mesh;
class Shader;
//struct aiMaterial;
//struct aiMesh;
//struct aiNode;
//struct aiScene;
//struct aiTextureType;
struct Texture;



class Model
{
public:
	// model data
	std::vector<Texture> loadedTextures; // Avoid loading textures multiple times
	std::vector<Mesh> meshes;
	std::string directory;
	bool gammaCorrection;
private:
    Vector3D position; // relative to Owning Actor. 

public:
    Model(std::string const& path, bool gamma = false);
    ~Model();
    // draws the model, and thus all its meshes
    void Draw(Shader& shader);

private:
    // loads a model with supported ASSIMP extensions from file and stores the resulting meshes in the meshes vector.
    void loadModel(std::string const& path);
	// processes a node in a recursive fashion. Processes each individual mesh located at the node and repeats this process on its children nodes (if any).
    void processNode(aiNode* node, const aiScene* scene);
    // processes a node in a recursive fashion. Processes each individual mesh located at the node and repeats this process on its children nodes (if any).
    Mesh processMesh(aiMesh* mesh, const aiScene* scene);
    // checks all material textures of a given type and loads the textures if they're not loaded yet.
    // the required info is returned as a Texture struct.
    std::vector<Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName);
    // Helper function
    unsigned int TextureFromFile(const char* path, const std::string& directory, bool gamma = false);

 
// -- Util Functions -- //
public:
    void setPosition(Vector3D _position) { position = _position; }
};



