#pragma once

#include <string>
#include <vector>

#include "GLFW/glfw3.h"
#include "../Include/glm-1.0.1/glm/glm.hpp"



// ---- Forward Declarations ---- //
class Model;
class Shader;

struct Vertex;
struct Texture;

struct Vertex
{
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec2 texCoords;
};

struct Texture
{
	unsigned int id;
	std::string type;
	std::string path;
};


/*
* Meshes work with vertex shaders with the following layout
* (layout 0) --> Vec3 Position 
* (layout 1) --> Vec3 Normal
* (layout 2) --> Vec2 Texture Coordinate
*/
class Mesh
{
// -- Members -- //
public:
	std::vector<Vertex>       vertices;
	std::vector<unsigned int> indices;
	std::vector<Texture>      textures;

private:
	unsigned int VAO, VBO, EBO;

// -- Member Functions & Constructors -- //
public:
	Mesh(std::vector<Vertex> _vertices, std::vector<unsigned int> _indices, std::vector<Texture> _textures);

	void Draw(Shader& shader);
};
