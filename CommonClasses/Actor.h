#pragma once

#include <string>
#include <sstream>

#include "Vector3D.h"
#include "Serialization/Serializeable.h"
#include "Model.h"
#include "Shader.h"



#define MAX_ACTORS 64


/*
* The Actor class initialises a static array of mesh pointers at runtime.
* This game isn't meant to be expanded upon once it's finished. As such,
* it is reasonable that the meshes can be cached in the only class that uses them
*/
class Actor
{
// -- Static Members -- //
private:
	/* ID's are shared across the network */
	static unsigned int nextId;
	/* cached models for constructing actor blueprints */
	static Model* modelCache[256];
	static unsigned int numCachedModels;

// -- Static Methods -- //
public:
	// Should be called before constructing any using create actor from blueprint method.
	static void loadModelCache(); 
	static Actor* createActorFromBlueprint(char blueprintId, Vector3D& position, Vector3D& rotation, unsigned int replicatedId, bool bUseReplicatedId = false); 

// -- Members -- //
private:
	const unsigned int id;
	Vector3D Position;
	Vector3D Rotation; // Should be a rotator. For now, is a Vector3D
	float moveSpeed;
	Vector3D moveDirection; // Normalized vector choosing which direction this actor moves
	Model* model;
	

// -- Methods, Constructors, Destructors -- //
public:
	// NOTE: These might be redundant since we will probably never need to construct an actor that isn't a blueprint
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
	void setPosition(Vector3D position) { Position = position; }

	Vector3D getRotation() { return Rotation; }
	void setRotation(Vector3D rotation) { Rotation = rotation; }

	float getMoveSpeed() { return moveSpeed; }
	void setMoveSpeed(float _moveSpeed) { moveSpeed = _moveSpeed; }

	Vector3D getMoveDirection() 
	{ 
		return moveDirection; 
	}
	void setMoveDirection(Vector3D _moveDirection) 
	{ 
		_moveDirection.Normalize();
		moveDirection = _moveDirection; 
	}
};