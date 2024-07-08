#pragma once

#include <string>
#include <glm\ext.hpp>

/*
* Wrapper class for compiling and running GLSL shader language programs on the GPU
*/
class Shader
{
public:
	// The program ID
	unsigned int ID;
	
	Shader(const char* szVertexPath, const char* szFragmentPath);

	void use();

	// -- Utility Functions -- //
	void setBool(const std::string& name, bool value) const;
	void setInt(const std::string& name, int value) const;
	void setFloat(const std::string& name, float value) const;
	void setVec3(const std::string& name, glm::vec3 value) const;
	void setVec3(const std::string& name, float x, float y, float z);
	void setMat4(const std::string& name, glm::mat4 value) const;
};