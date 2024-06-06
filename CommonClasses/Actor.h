#pragma once

#include "Vector.h"
#include "Serialization/Serializeable.h"

static unsigned int NextID;

/*
* This actor class implements the networking interface
*/
class Actor : public Serializable
{
// -- Members -- //
private:
	Vector3D Position;
	Vector3D Rotation; // Should be a rotator. For now, is a
	//Mesh mesh; TODO 

// -- Methods, Constructors, Destructors -- //
public:
	Actor(Vector3D position, Vector3D rotation) : Position(position), Rotation(rotation) {};
	/* Serializes this objects network required data */
	int serialize(const char* buffer) const override;

	Actor* deserialize(const char* buffer) override;
	
public:
	// --  Getters and Setters -- //
	inline Vector3D getPosition() { return Position; }
	inline void setPosition(Vector3D position) { Position = position.Normalize(); }

	inline Vector3D getRotation() { return Rotation; }
	inline void setRotation(Vector3D rotation) { Rotation = rotation.Normalize(); }
};