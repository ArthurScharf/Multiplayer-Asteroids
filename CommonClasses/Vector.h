#pragma once

class Vector3D
{
public:
	float x;
	float y;
	float z;

	Vector3D(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {};

	Vector3D operator+(Vector3D& other) { return Vector3D(x + other.x, y + other.y, z + other.z); }
	Vector3D operator+(float val) { return Vector3D(x + val, y + val, z + val);  }
	void operator+=(Vector3D& other) { x += other.x; y += other.y; z += other.z; };
	void operator+=(float val) { x += val; y += val; z += val; };
};