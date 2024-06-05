#pragma once
#include <math.h>


class Vector3D
{
public:
	float x;
	float y;
	float z;

	Vector3D() : x(0.f), y(0.f), z(0.f) {};
	Vector3D(float val) : x(val), y(val), z(val) {};
	Vector3D(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {};

	/* Normalizes the vector 
	Is ternary operator the best way to check for 0?
	*/
	void Normalize()
	{
		float magnitude = sqrt((x * x) + (y * y) + (z * z));
		if (magnitude == 0) return; // magnitude is only 0 if all values are 0
		x /= magnitude;
		y /= magnitude;
		z /= magnitude;
	};

	std::string toString()
	{
		char buffer[50]{};
		sprintf_s(buffer, 50, "%6.5f %6.5f %6.5f", x, y, z);
		return std::string(buffer);
	}

	// TODO: order these in order the order in which they operate. As an exercise
	Vector3D operator+(Vector3D& other) { return Vector3D(x + other.x, y + other.y, z + other.z); }
	Vector3D operator+(float val) { return Vector3D(x + val, y + val, z + val);  }
	void operator+=(Vector3D& other) { x += other.x; y += other.y; z += other.z; };
	void operator+=(Vector3D other) { x += other.x; y += other.y; z += other.z; };
	void operator+=(float val) { x += val; y += val; z += val; };
};


static Vector3D& operator*(Vector3D& v, float f) 
{
	v.x *= f;
	v.y *= f;
	v.z *= f;
	return v;
}