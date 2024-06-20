
#include "Actor.h"
#include "pch.h"

#include <iostream>
#include <cstdio>


Actor::Actor(Vector3D position, Vector3D rotation)
{
	id = nextId++;
	Position = position;
	Rotation = rotation;
	model = nullptr;
}


Actor::Actor(Vector3D position, Vector3D rotation, unsigned int replicatedId)
{
	id = replicatedId;
	Position = position;
	Rotation = rotation;
	model = nullptr;
}


unsigned int Actor::serialize(char* buffer, Actor* actor)
{
	unsigned int bufferLen = sizeof(Actor); // Null terminating
	
	 std::cout << std::endl;

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