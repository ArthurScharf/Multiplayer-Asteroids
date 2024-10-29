#include "pch.h"
#include "Vector3D.h"

Vector3D::Vector3D() 
	: x(0.f), y(0.f), z(0.f)
{};


Vector3D::Vector3D(float val)
	: x(val), y(val), z(val)\
{};


Vector3D::Vector3D(float _x, float _y, float _z) 
	: x(_x), y(_y), z(_z) 
{};


float Vector3D::length()
{
	return sqrt(pow(x, 2) + pow(y, 2) + pow(z, 2));
}


void Vector3D::Normalize()
{
	{
		float magnitude = sqrt((x * x) + (y * y) + (z * z));
		if (magnitude == 0) return; // magnitude is only 0 if all values are 0
		x /= magnitude;
		y /= magnitude;
		z /= magnitude;
		// return this; // Cascading
	}
}


Vector3D Vector3D::getNormal()
{
	float magnitude = sqrt((x * x) + (y * y) + (z * z));
	if (magnitude == 0) return Vector3D(0.f);
	return Vector3D(x / magnitude, y / magnitude, z / magnitude);
}


void Vector3D::zero()
{
	x = y = z = 0.f;
}