#pragma once

#include <string>
#include <sstream>

#include "Vector.h"
#include "Serialization/Serializeable.h"



/* ID's are shared across the network */
static unsigned int nextId;

/*
* This actor class implements the networking interface
*/
class Actor
{
// -- Members -- //
private:
	unsigned int id; // 4
	Vector3D Position; // 12
	Vector3D Rotation; // Should be a rotator. For now, is a Vector3D
	//Mesh mesh; TODO 

// -- Methods, Constructors, Destructors -- //
public:
	Actor(Vector3D position, Vector3D rotation); // Server side
	Actor(Vector3D position, Vector3D rotation, unsigned int replicatedId); // Client side

	std::string toString()
	{
		std::ostringstream stream;
		stream << "ID: " << id;
		stream << " / Position: " << Position.toString();
		stream << " / Rotation: " << Rotation.toString();
		return stream.str();
	}

	static unsigned int serialize(char* buffer, Actor* actor);
	/*
	* The signature is different than serialize because we want to instantiate an actor internally.
	* If we allowed the return actor to be a parameter, we allow a user to pass any kind of actor pointer,
	* which run's it's own selection of issues
	*/
	static Actor* deserialize(char* buffer, unsigned int& bytesRead); 

// --  Getters and Setters -- //
public:
	unsigned int getId() { return id; }

	inline Vector3D getPosition() { return Position; }
	inline void setPosition(Vector3D position) 
	{
		position.Normalize();
		Position = Position + position;
	}

	inline Vector3D getRotation() { return Rotation; }
	inline void setRotation(Vector3D rotation)
	{
		rotation.Normalize();
		Rotation = Position + rotation;
	}
};