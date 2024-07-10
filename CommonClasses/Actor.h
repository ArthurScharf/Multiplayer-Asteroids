#pragma once

#include <string>
#include <sstream>

#include "Vector3D.h"
#include "Serialization/Serializeable.h"
#include "Model.h"
#include "Shader.h"





/*
* This actor class implements the networking interface
*/
class Actor
{
// -- Members -- //
private:
	/* ID's are shared across the network */
	static unsigned int nextId;
	const unsigned int id; // 4
	Vector3D Position; // 12
	Vector3D Rotation; // Should be a rotator. For now, is a Vector3D
	//Mesh mesh; TODO 
	Model* model;

// -- Methods, Constructors, Destructors -- //
public:
	Actor(Vector3D& position, Vector3D& rotation); // Server side
	Actor(Vector3D& position, Vector3D& rotation, unsigned int replicatedId); // Client side
	~Actor();

	std::string toString();

	// Separate from cstr because models shouldn't always be loaded
	void InitializeModel(const std::string& path);

	void Draw(Shader& shader);

	static unsigned int serialize(char* buffer, Actor* actor);
	/*
	* The signature is different than serialize because we want to instantiate an actor internally.
	* If we allowed the return actor to be a parameter, we allow a user to pass any kind of actor pointer,
	* which run's it's own selection of issues
	*/
	static Actor* deserialize(const char* buffer, unsigned int& bytesRead); 



// --  Getters and Setters -- //
public:
	unsigned int getId() { return id; }

	Vector3D getPosition() { return Position; }
	void setPosition(Vector3D position) 
	{
		//position.Normalize();
		Position = position;
	}

	Vector3D getRotation() { return Rotation; }
	void setRotation(Vector3D rotation)
	{
		//rotation.Normalize();
		Rotation = rotation;
	}
};