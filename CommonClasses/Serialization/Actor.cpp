
#include "Actor.h"
#include "pch.h"

#include <iostream>
#include <cstdio>


unsigned int Actor::nextId = 0;
unsigned int Actor::numCachedModels = 0;
Model* Actor::modelCache[256];
bool Actor::bHasInitializedModelCache = false;


void Actor::loadModelCache()
{
	modelCache[numCachedModels++] = new Model("C:/Users/User/source/repos/Multiplayer-Asteroids/CommonClasses/FBX/Gear/Gear1.fbx");
	std::cout << "Actor::loadModelCache -- \"Default\" loaded" << std::endl;
	modelCache[numCachedModels++] = new Model("C:/Users/User/source/repos/Multiplayer-Asteroids/CommonClasses/FBX/Gear/Gear1.fbx");
	std::cout << "Actor::loadModelCache -- \"Player Character\" loaded" << std::endl;
	modelCache[numCachedModels++] = new Model("C:/Users/User/source/repos/Multiplayer-Asteroids/CommonClasses/FBX/Asteroid/Asteroid.fbx");
	std::cout << "Actor::loadModelCache -- \"Asteroid\" loaded" << std::endl;
	modelCache[numCachedModels++] = new Model("C:/Users/User/source/repos/Multiplayer-Asteroids/CommonClasses/FBX/chair/chair.fbx");
	std::cout << "Actor::loadModelCache -- \"Projectile\" Loaded" << std::endl;

	bHasInitializedModelCache = true;
}



Actor* Actor::netDataToActor(ActorNetData data)
{
	Actor* actor = new Actor(data.Position, data.Rotation, data.blueprintID, true, data.id);
	return actor;
}


Actor::Actor(Vector3D _position, Vector3D _rotation, EActorBlueprintID _blueprintID, bool bSetModel, unsigned int _id)
	: Position(_position), Rotation(_rotation), Scale(1.f), id(_id), blueprintID(_blueprintID)
{
	if (bSetModel)
	{
		/* -- NOTE --
		Cached models are stored at the same index as their corresponding EActorBlueprintID value.
		Thus, we can use the value of blueprintID to find the correct model, including default models
		*/
		model = modelCache[blueprintID];
	}
	else
	{
		model = nullptr;
	}

	switch (blueprintID)
	{
	case ABI_PlayerCharacter:
	{
		moveSpeed = 70.f;
		moveDirection.zero();
		break;
	}
	case ABI_Projectile:
	{
		moveSpeed = 30.f; // TEMP. 120.f
		moveDirection = _rotation;
		break;
	}
	case ABI_Asteroid:
	{
		Scale = 0.2; // Hardcode since asteroid model will never change
		moveSpeed = 50.f;
		moveDirection  = _rotation;
		break;
	}
	default:
	{
		// Is warning bc we may need a reason to create actor with custom settings.
		printf("WARNING::Actor::Actor -- no blueprint type was passed. Requires manual construction");
		moveSpeed = 0.f;
		moveDirection.zero();
		break;
	}
	}
}


Actor::~Actor()
{
	// if (model) delete model; // BUG: I'm getting a memory error related to how the memory is aligned, it seems. I'll allow a memory leak for now
}



void Actor::addToPosition(Vector3D addend)
{
	Position.x += addend.x;
	Position.y += addend.y;
	Position.z += addend.z;
}


std::string Actor::toString()
{
	std::ostringstream stream;
	stream << "ID: " << id;
	stream << " / Position: " << Position.toString();
	stream << " / Rotation: " << Rotation.toString();
	return stream.str();
}



void Actor::Draw(Shader& shader)
{
	if (!model)
	{
		std::cout << "ERROR::Actor::Draw -- !model\n";
		return;
	}
	shader.setVec3("positionOffset", glm::vec3(Position.x, Position.y, Position.z));
	shader.setFloat("scale", Scale);
	model->Draw(shader);
}


ActorNetData Actor::toNetData()
{
	ActorNetData data;
	data.id = id;
	data.blueprintID = blueprintID;
	data.moveDirection = moveDirection;
	data.moveSpeed = moveSpeed;
	data.Position = Position;
	data.Rotation = Rotation;
	return data;
}


unsigned int Actor::serialize(char* buffer, Actor* actor)
{
	unsigned int bufferLen = sizeof(ActorNetData); // Null terminating
	
	unsigned int actorId = actor->getId(); 
	memcpy(buffer, &actorId, sizeof(unsigned int));
	buffer += sizeof(unsigned int);

	//std::cout << "actorId: " << actorId << std::endl;
	//buffer -= sizeof(unsigned int);
	//unsigned int testId;
	//memcpy(&testId, buffer, sizeof(unsigned int));
	//std::cout << "testId: " << actorId << std::endl;
	//buffer += sizeof(unsigned int);

	// Setting Position Bytes
	Vector3D pos = actor->getPosition();
	memcpy(buffer, &pos, sizeof(Vector3D));
	buffer += sizeof(Vector3D);

	//std::cout << "pos: " << pos.toString() << std::endl;
	//buffer -= sizeof(float) * 3;
	//Vector3D testPos;
	//memcpy(&testPos, buffer, sizeof(Vector3D));
	//std::cout << "testPos: " << testPos.toString() << std::endl;
	//buffer += sizeof(Vector3D);

	// Setting Rotation Bytes
	Vector3D rot = actor->getRotation();
	memcpy(buffer, &rot, sizeof(Vector3D));
	buffer += sizeof(Vector3D);

	//std::cout << "rot: " << rot.toString() << std::endl;
	//buffer -= sizeof(float) * 3;
	//Vector3D testRot;
	//memcpy(&testRot, buffer, sizeof(Vector3D));
	//buffer += sizeof(Vector3D);
	//std::cout << "testRot: " << testRot.toString() << std::endl;
	//std::cout << std::endl;

	Vector3D movDir = actor->getMoveDirection();
	memcpy(buffer, &movDir, sizeof(Vector3D));

	return bufferLen;
}


/* This function doesn't care which bytes are read. Unsafe */
ActorNetData Actor::deserialize(const char* buffer, unsigned int& bytesRead)
{
	ActorNetData netData;
	bytesRead = sizeof(netData);
	memcpy(&netData, buffer, bytesRead);
	return netData;
}