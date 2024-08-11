
#include "Actor.h"
#include "pch.h"

#include <iostream>
#include <cstdio>


unsigned int Actor::nextId = 0;
unsigned int Actor::numCachedModels = 0;
Model* Actor::modelCache[256];


void Actor::loadModelCache()
{
	modelCache[numCachedModels] = new Model("C:/Users/User/source/repos/Multiplayer-Asteroids/CommonClasses/FBX/Gear/Gear1.fbx");
	modelCache[numCachedModels] = new Model("C:/Users/User/source/repos/Multiplayer-Asteroids/CommonClasses/FBX/chair/chair.fbx");
	numCachedModels = 2;
}


Actor* Actor::createActorFromBlueprint(char blueprintId, Vector3D position, Vector3D rotation, unsigned int replicatedId, bool bClient)
{
	std::string path = "";
	float moveSpeed;
	Vector3D moveDirection;

	switch (blueprintId)
	{
		case ABP_GEAR: // 0 --> Gear
		{
			path = "C:/Users/User/source/repos/Multiplayer-Asteroids/CommonClasses/FBX/Gear/Gear1.fbx";
			moveSpeed = 70.f; // Character movespeed
			moveDirection.zero();
			break;
		}
		case ABP_PROJECTILE: // 1 --> Projectile
		{
			path = "C:/Users/User/source/repos/Multiplayer-Asteroids/CommonClasses/FBX/chair/chair.fbx";
			moveSpeed = 120.f; // projectile movespeed
			moveDirection = rotation;
			break;
		}
		default:
		{
			break;
		}
	}

	Actor* actor = (bClient) ? new Actor(position, rotation, replicatedId) : new Actor(position, rotation);
	actor->setMoveSpeed(moveSpeed);
	actor->setMoveDirection(moveDirection);
	if (bClient && path != "")
	{
		Model* temp = new Model(path);
		if (temp) actor->model = temp;
	}
	return actor;
}


Actor* Actor::netDataToActor(ActorNetData data)
{
	Actor* actor = new Actor(data.Position, data.Rotation, data.id);
	actor->setMoveDirection(data.moveDirection);
	return actor;
}

Actor::Actor(Vector3D& position, Vector3D& rotation)
	: Position(position), Rotation(rotation), id(nextId++), moveDirection(0.f), moveSpeed(0.f)
{}


Actor::Actor(Vector3D& position, Vector3D& rotation, unsigned int replicatedId)
	 : Position(position), Rotation(rotation), id(replicatedId), moveDirection(0.f), moveSpeed(0.f)
{
	model = nullptr; // Why is this insufficient to avoid a bug from loading? If I use InitializeModel() with it failing, the system doesnt throw any exceptions

	/*
	* BUG
	* I think what's happening is this:
	* Model being nullptr causes some kind of problem within a vector somewhere.
	* Even a failed call InitializeModel will result in a non-null value
	* This non-null value avoids the problem
	*/
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


void Actor::InitializeModel(const std::string& path)
{
	Model* temp = new Model(path);
	if (temp) model = temp;
}

void Actor::Draw(Shader& shader)
{
	if (!model)
		std::cout << "ERROR::Actor::Draw -- !model\n";
	shader.setVec3("positionOffset", glm::vec3(Position.x, Position.y, Position.z));
	model->Draw(shader);
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
	bytesRead = sizeof(unsigned int) + (3 * sizeof(Vector3D));
	memcpy(&netData, buffer, bytesRead);
	return netData;
}