#pragma once
#include <math.h>
#include <string>


/*
* Should probably be a struct
*/
class Vector3D
{
public:
	float x;
	float y;
	float z;

	Vector3D();
	Vector3D(float val);
	Vector3D(float _x, float _y, float _z);


	float length();

	/* Normalizes the vector 
	Is ternary operator the best way to check for 0?
	*/
	void Normalize();

	std::string toString()
	{
		char buffer[50]{};
		sprintf_s(buffer, 50, "%4.6f %4.6f %4.6f", x, y, z);
		return std::string(buffer);
	}

	// TODO: order these in order the order in which they operate. As an exercise
	Vector3D operator+(Vector3D other) { return Vector3D(x + other.x, y + other.y, z + other.z); }
	//Vector3D operator+(Vector3D& other) { return Vector3D(x + other.x, y + other.y, z + other.z); }
	Vector3D operator+(float val) { return Vector3D(x + val, y + val, z + val);  }
	void operator+=(Vector3D& other) { x += other.x; y += other.y; z += other.z; };
	void operator+=(Vector3D other) { x += other.x; y += other.y; z += other.z; };
	void operator+=(float val) { x += val; y += val; z += val; };
	bool operator==(Vector3D other) { return (x == other.x && y == other.y && z == other.z); }
	bool operator!=(Vector3D other) { return (x != other.x && y != other.y && z != other.z); }
};


//static Vector3D& operator*(Vector3D& v, float f) 
//{
//	v.x *= f;
//	v.y *= f;
//	v.z *= f;
//	return v;
//}

static Vector3D operator*(Vector3D v, float f)
{
	v.x *= f;
	v.y *= f;
	v.z *= f;
	return v;
}

static Vector3D operator*(float f, Vector3D v)
{
	v.x *= f;
	v.y *= f;
	v.z *= f;
	return v;
}