#pragma once


/*
* An interface for a class to define how it is serialized
*/
class Serializable
{
public:
	// Returns the number of bytes placed in the buffer. Initializes the memory for the buffer
	virtual int serialize(const char* buffer) const = 0;
	static Serializable* deserialize(const char* buffer);
};

