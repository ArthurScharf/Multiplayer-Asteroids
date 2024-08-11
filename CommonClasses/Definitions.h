#pragma once


#define MAX_PLAYERS 4




/*
* Single call to memcpy and makes code more readable.
* How much computational overhead does this approach incur?
*/
struct NetworkSpawnData
{
private:
	char messageType = 's';
public:
	unsigned int simulationStep; // renamed from stateSequenceID
	unsigned int dummyActorID; // ID used to link dummy with authoritative client actor
	unsigned int networkedActorID;
};