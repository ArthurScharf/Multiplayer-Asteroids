#pragma once

#include <string>
#include <sstream>

#include "Vector3D.h"
#include "Serialization/Serializeable.h"
#include "Model.h"
#include "Shader.h"






// TODO: Why didn't I want to use the ENUMS?
//enum ActorBlueprint
//{
//	ABP_GEAR,
//	ABP_CHAIR
//};
#define ABP_GEAR '\x0'
#define ABP_PROJECTILE '\x1'





/*
Used to identify which model and actor parameter values are to be set.
Used as index for model cache when setting actor's model ptr.
*/
enum EActorBlueprintID
{
	ABI_Default,
	ABI_PlayerCharacter,
	ABI_Asteroid,
	ABI_Projectile
};


/*
* - Serves as a schema for reading actor data from a stream without constructing an entire actor
* - Can be used for detecting required size when working with net buffers
* 
While it might be true that I could just serialize an actor directly, having this struct allows the data being serialized
over the network to easily differ from the data that would be serialized if we were just using the actor class
for size checks
*/
struct ActorNetData
{
	unsigned int id; // 0 is never assigned. Reserved as a sort of `null` state for id.
	EActorBlueprintID blueprintID; // Used to identify which actor blueprint this is
	Vector3D Position;
	Vector3D Rotation; // Should be a rotator. For now, is a Vector3D
	Vector3D moveDirection; // Normalized vector choosing which direction this actor moves
	float moveSpeed;
	bool bIsDestroyed; // Used to identify actors that are to be destroyed locally by the client

	ActorNetData() :
		id(0),
		blueprintID(ABI_Default),
		Position(0.f),
		Rotation(0.f),
		moveDirection(0.f),
		moveSpeed(0.f),
		bIsDestroyed(false) 
	{}
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
	/*
	* Mask for seperating the id of an actor from the network ownership of that actor.
	* last two bits in an unsigned long are reserved to identify the owning client
	* 
	* bit starts at pos 0. Shift 28 to put in pos 29. -1 gives max number in 28 bits
	*/
	static const unsigned int idMask = (1 << 28) - 1; 

	
	/* cached models for constructing actor blueprints */
	static Model* modelCache[256]; // TODO: 256 is too large. Should be shrunk massively once we know how many we're actually using
	static unsigned int numCachedModels;
	static bool bHasInitializedModelCache;

// -- Static Methods -- //
public:
	// Should be called before constructing any using create actor from blueprint method.
	static void loadModelCache(); 

	/* -- Client Use Only -- */
	static Actor* netDataToActor(ActorNetData data);


// -- Members -- //
private:
	unsigned int id; // network ID
	EActorBlueprintID blueprintID;
	Vector3D Position;
	Vector3D Rotation; // Should be a rotator. For now, is a Vector3D
	float Scale;
	Vector3D moveDirection; // Normalized vector choosing which direction this actor moves
	float moveSpeed;
	Model* model;
	float collisionRadius;

// -- Methods, Constructors, Destructors -- //
public:
	

	/* 
	* 
	* bSetModel : server doesn't render so this can be set to false
	* _id : allows for control of the ID being set. Called by client when creating a proxy actor
	*/
	Actor(unsigned int _id, Vector3D _position, Vector3D _rotation, EActorBlueprintID _blueprintID = ABI_Default, bool bSetModel = false);
	


	~Actor();




	void addToPosition(Vector3D addend);

	std::string toString();

	void Draw(Shader& shader);

	ActorNetData toNetData();

	// DEPRECATED. May want to refactor to emplace a ActorNetData into the passed buffer
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
	
	void setModel(Model* _model) { model = _model; }

	float getCollisionRadius() { return collisionRadius; }

	EActorBlueprintID getBlueprintID() { return blueprintID; }

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




