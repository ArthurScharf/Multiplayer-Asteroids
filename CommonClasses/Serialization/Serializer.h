#pragma once

#include <functional>
#include <map>
#include <string>

#include "../Actor.h"
#include "../Definitions.h"
#include "Serializeable.h"


/* ----- Format ------
* Byte(s) : Data
* 0-23    : type_info
* 2+      : raw bytes. The serializer should already know how to read the byte stream
*/




/*
* 1. Data comes in with schema ID. --> The next n bytes are formatted in a way that is unique,
*                                      to the object that has been serialized. 
* 2. --> Deserialization parameters are unique for each type of serializable data
* 3. --> Each type of serializable MUST have it's own algorithm
* 
* Solution: Serializer class switches on type bit to decide which static classes serializer it
*           should call.
*
* Problem:  A lot of includes for each type of serializable data. Minimal issue for this game
* Problem:  Must be maintined by me. I'd rather their be a dynamic solution. Again, at the scale
*           of the project, this is very tolerable
* Problem:  Each subclass of a class that is implementing Serializable will require
*           it's own switch case. Again, this is neither scalable nor sustainable within
*           larger systems
* 
* Conclusion: We're gonna solve this problem by doing it statically. In the future,
*             we'll want to find a better solution.
*/


class Factory;
class ActorFactory;


class Deserializer
{

	/*
	* Each buffer is packaged by by type. This makes unpacking easier
	*/
	static void deserialize(const char* buffer, unsigned int bufferLen)
	{
		unsigned int bufferTypeId = *buffer++; // Scheme --> First bit is the type id
		if (bufferTypeId == 0) // Instruction
		{

		}
		else if (bufferTypeId == 1) // Actor Update
		{
			for (unsigned int i = 0; i < bufferLen - 1; i++)
			{
				unsigned int actorId = *(++buffer); // Retreiving the actor ID

			}
		}
	}
};



