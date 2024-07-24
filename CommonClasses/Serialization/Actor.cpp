
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


Actor* Actor::createActorFromBlueprint(char blueprintId, Vector3D& position, Vector3D& rotation, unsigned int replicatedId, bool bUseReplicatedId)
{
	std::string path = "";
	float moveSpeed;
	switch (blueprintId)
	{
		case '\x0': // 0 --> Gear
		{
			path = "C:/Users/User/source/repos/Multiplayer-Asteroids/CommonClasses/FBX/Gear/Gear1.fbx";
			moveSpeed = 70.f; // Character movespeed
			break;
		}
		case '\x1': // 1 --> Chair
		{
			path = "C:/Users/User/source/repos/Multiplayer-Asteroids/CommonClasses/FBX/chair/chair.fbx";
			moveSpeed = 120.f; // projectile movespeed
			break;
		}
		default: // Models don't need to be loaded if the server is invoking this method.
		{
			break;
		}
	}

	Actor* actor = (bUseReplicatedId) ? new Actor(position, rotation, replicatedId) : new Actor(position, rotation);
	actor->setMoveSpeed(moveSpeed);
	if (path != "")
	{
		Model* temp = new Model(path);
		if (temp) actor->model = temp;
	}
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
	unsigned int bufferLen = sizeof(Actor); // Null terminating
	
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

	return bufferLen;
}


/* actor is an */
Actor* Actor::deserialize(const char* buffer, unsigned int& bytesRead)
{

	// Actor id
	unsigned int id;
	memcpy(&id, buffer, sizeof(unsigned int));
	buffer += sizeof(unsigned int);

	// Actor position
	Vector3D pos;
	memcpy(&pos, buffer, sizeof(Vector3D));
	buffer += sizeof(Vector3D);

	// Actor rotation
	Vector3D rot;
	memcpy(&rot, buffer, sizeof(Vector3D));

	return new Actor(pos, rot, id);
}