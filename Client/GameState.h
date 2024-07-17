#pragma once

#include "../CommonClasses/Actor.h"

class GameState
{
private:
	// Used to sequence incoming states
	unsigned int sequenceId;
	// Backing array
	Actor* actors[MAX_ACTORS];
	unsigned int numActors; 


public:
	/*
	* Deserializes the state number and actors and sets its members using this data
	*/
	GameState(unsigned int _sequenceId, Actor* _actors[MAX_ACTORS])
		: sequenceId(_sequenceId), numActors(0)
	{	
		for (int i = 0; i < MAX_ACTORS && _actors[i] != nullptr; i++)
		{
			actors[i] = (Actor*)malloc(sizeof(Actor*));
			actors[i] = _actors[i];
			numActors++;
		}
	};


	// -- Utility Functions & Operators -- //
public:
	// const Actor** getActors() { return actors; }
	unsigned int getSequenceId() { return sequenceId; }

	bool operator==(GameState other) { return sequenceId == other.sequenceId; };
};