#pragma once


#define MAX_PLAYERS 4







// ---- Message Types ---- //
#define MSG_CONNECT 'c'		// Connection request/confirmation
#define MSG_TSTEP 't'		// Simulation Step request/reply
#define MSG_REP 'r'			// Client state change request / server state replication
#define MSG_ID 'i'			// 
#define MSG_SPAWN 's'		// Client projectile Spawn / Server proxy link 
#define MSG_EXIT 'e'		// Client --> Server. Instructs server to stop
#define MSG_RPC  'p'        // Remote Procedure Call


#define RPC_TEST 'x'		// For Debugging



/*
* TODO: Use an enum like this instead. Prevents having to manually lookup which characters
* have already been used when deciding on char's for new messages.
* Additionally, allows for a greater number of messages to exist
*/
//enum Message
//{
//	MSG_CONNECT,
//	MSG_TSTEP,
//	MSG_REP,
//	MGS_ID,
//	MSG_SPAWN,
//	MSG_EXIT
//};



/*
* The Header for all RPC messages.
* Intended to be followed with a message containing the data required for the RPC to be called
*/
struct RemoteProcedureCall
{
	/* Which RPC will be called*/
	char method; 

	/* Simulation Step the RPC is being called during */
	unsigned int simulationStep; 

	/* Seconds after the last fixed frequency update completed */
	float secondsSinceLastUpdate;

	char message[20];
};







/* TODO: Modify this to hold only the data required for a certain RPC */
struct NetworkSpawnData
{
private:
	char messageType = 's';
public:
	unsigned int simulationStep; // renamed from stateSequenceID
	unsigned int dummyActorID; // ID used to link dummy with authoritative client actor
	unsigned int networkedActorID;
};




