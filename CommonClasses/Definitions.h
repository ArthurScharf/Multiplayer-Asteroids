#pragma once


#define MAX_PLAYERS 4




// ---- Message Types ---- //
#define MSG_CONNECT 'c'		// Connection request/confirmation
#define MSG_TSTEP 't'		// Simulation Step request/reply
#define MSG_REP 'r'			// Client state change request / server state replication
#define MSG_ID 'i'			// 
#define MSG_SPAWN 's'		// Client projectile Spawn / Server proxy link 
#define MSG_EXIT 'e'		// Client --> Server. Instructs server to stop



/* TODO: This should be named to make it clear that it's only purpose is to link proxies
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