#pragma once


#include "GameState.h"

#if _DEBUG
#include <string> 
#endif

#define MAX_STATES 20 // This number needs to be decided on based upon the rate at which states are created and consumed

class CircularBuffer
{
private:
	GameState* states[MAX_STATES];
	/* Index of state that is to be overwritten upon append */
	int index;
	bool bIsFull;

public:
	CircularBuffer()
	{
		for (int i = 0; i < MAX_STATES; i++)
		{
			states[i] = nullptr;
		}
		index = 0;
		bIsFull = false;
	};

	void append(GameState* state)
	{
		states[index] = state;
		index++;
		if (index >= MAX_STATES)
		{
			index = 0;
			bIsFull = true;
		}
	};


#if _DEBUG
	std::string toString()
	{
		std::stringstream strm;
		for (int i = 0; i < (bIsFull ? MAX_STATES : index); i++)
		{
			if (!states[i])
			{
				printf("ERROR / CircularBuffer::toString / states[%d] == nullptr", i);
				continue;
			}
			strm << states[i]->getSequenceId() << " ";
		}
		strm << "\n";

		return strm.str();
	}
#endif

};