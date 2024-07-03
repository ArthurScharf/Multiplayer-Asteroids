#pragma once

#include <string>
#include <vector>

#include "Shader.h"

//#include <glm-1.0.1/glm/glm.hpp> // GL math
#include "GLFW/glfw3.h"
#include "../Include/glm-1.0.1/glm/glm.hpp"

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
	Mesh(std::vector<Vertex> _vertices, std::vector<unsigned int> _indices, std::vector<Texture> _textures)
	{
		vertices = _vertices;
		indices = _indices;
		textures = _textures;
		
		glGenVertexArrays(1, &VAO); // Returns ID for vertex Array stored inside OpenGL
		glGenBuffers(1, &VBO); // Generate VBO object that will be stored on the GPU
		glGenBuffers(1, &EBO); // Generate an Element Buffer object that will be stored on the GPU

		glBindVertexArray(VAO); // Binds the VAO
		glBindBuffer(GL_ARRAY_BUFFER, VBO); // Binds VBO. Since VAO was bound first, VBO is associated with the VAO
		
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW); // Sends the vertex data to the buffer

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

		// Vertex positions
		glEnableVertexAttribArray(0); // attribute 0 will be vertices
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
		// Vertex Normals
		glEnableVertexAttribArray(1); // attribute 1 will be vertex normals
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
		// Texture Coordinates
		glEnableVertexAttribArray(2); // attribute 2 will be texture coordinates
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoords));

		glBindVertexArray(0); // Resetting the bind is just good practice
	}

	void Draw(Shader& shader)
	{
		unsigned int diffuseIndex = 1;
		unsigned int specularIndex = 1;
		for (unsigned int i = 0; i < textures.size(); i++)
		{
			glActiveTexture(GL_TEXTURE + i); // activate proper texture unit before binding
			std::string index;
			std::string textureType = textures[i].type;
			if (textureType == "texture_diffuse")
				index = std::to_string(specularIndex++);
			else if (textureType == "texture_specular")
				index = std::to_string(specularIndex++);

			shader.setInt(("material." + textureType + index).c_str(), i);
			glBindTexture(GL_TEXTURE_2D, textures[i].id);
		}
		glActiveTexture(GL_TEXTURE0);

		// Draw mesh
		glBindVertexArray(VAO);
		glDrawElements(GL_TRIANGLES, (GLsizei)indices.size(), GL_UNSIGNED_INT, 0);
		glBindVertexArray(0); // good practice to reset the VAO binding
	}
};
