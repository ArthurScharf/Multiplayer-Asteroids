#pragma once

#include <string>
#include <sstream>

#include "Vector3D.h"
#include "Serialization/Serializeable.h"
#include "Model.h"
#include "Shader.h"



#define MAX_ACTORS 64



// TODO: Why didn't I want to use the ENUMS?
//enum ActorBlueprint
//{
//	ABP_GEAR,
//	ABP_CHAIR
//};
#define ABP_GEAR '\x0'
#define ABP_PROJECTILE '\x1'


/*
* - Serves as a schema for reading actor data from a stream without constructing an entire actor
* - Can be used for detecting required size when working with net buffers
*/
struct ActorNetData
{
	unsigned int id;
	// char actorBlueprintType;
	Vector3D Position;
	Vector3D Rotation; // Should be a rotator. For now, is a Vector3D
	Vector3D moveDirection; // Normalized vector choosing which direction this actor moves
};


/*
* Used by client and server to link unlinked proxies with their authoritative counterparts
*/
struct proxyLinkPackage
{
	unsigned int proxyLinkID;
	unsigned int actorNetworkID; 
};


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

	/*
	* TODO:
	* Client doesn't always replicate id. this function makes it unclear when it should be called
	*/
	static Actor* createActorFromBlueprint(char blueprintId, Vector3D position, Vector3D rotation, unsigned int replicatedId, bool bClient = false); 
	static Actor* netDataToActor(ActorNetData data);


// -- Members -- //
private:
	unsigned int id; // prefix const
	Vector3D Position;
	Vector3D Rotation; // Should be a rotator. For now, is a Vector3D
	Vector3D moveDirection; // Normalized vector choosing which direction this actor moves
	float moveSpeed;
	Model* model;
	

// -- Methods, Constructors, Destructors -- //
public:
	// NOTE: These might be redundant since we will probably never need to construct an actor that isn't a blueprint
	Actor(Vector3D& position, Vector3D& rotation); // Server side
	Actor(Vector3D& position, Vector3D& rotation, unsigned int replicatedId); // Client side
	~Actor();


	void addToPosition(Vector3D addend);

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
	static ActorNetData deserialize(const char* buffer, unsigned int& bytesRead); 



// --  Getters and Setters -- //
public:
	unsigned int getId() { return id; }
	// WARNING: Should only ever be called by Client when linking proxies
	void setId(unsigned int _id) { id = _id; } 

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

// -- Operators -- //
	bool operator==(Actor* other)
	{
		// NOTE: This is the same data as NetActorData
		return id == other->id && \
			Position == other->Position && \
			Rotation == other->Rotation && \
			moveDirection == other->moveDirection;
	}
};




