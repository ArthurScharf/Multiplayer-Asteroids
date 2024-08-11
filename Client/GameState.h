#pragma once

#include "../CommonClasses/Actor.h"

class GameState
{
private:
	// Used to sequence incoming states
	unsigned int sequenceId;
	// Backing vector
	std::vector<Actor*> actors;


public:
	GameState(unsigned int _sequenceId, std::vector<Actor*> _actors)
		: sequenceId(_sequenceId), actors(_actors)
	{};


	/*
	* Checks for true equality between this state and the other.
	*/
	bool IsEqual(GameState* other)
	{
		if (actors.size() != other->actors.size()) 
			return false;

		bool result = true;
		result &= sequenceId == other->sequenceId;


		// -- Sorting this -- //
		std::sort(
			actors.begin(),
			actors.end(),
			[](Actor* a, Actor* b)
			{
				return a->getId() > b->getId();
			}
		);

		// -- Sorting Other -- //
		std::sort(
			other->actors.begin(),
			other->actors.end(),
			[](Actor* a, Actor* b)
			{
				return a->getId() > b->getId();
			}
		);

		for (int i = 0; i < actors.size(); i++)
		{
			result &= actors[i] == other->actors[i];
		}
		return result;
	}



	// -- Utility Functions & Operators -- //
public:
	// const Actor** getActors() { return actors; }
	unsigned int getSequenceId() { return sequenceId; }

	bool operator==(GameState other) { return sequenceId == other.sequenceId; };
};