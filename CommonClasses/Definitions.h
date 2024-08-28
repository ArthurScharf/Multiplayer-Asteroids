#pragma once

// --------------------------------------------------- //
// ---------- General Structs & Definitions ---------- //
// --------------------------------------------------- //
#define MAX_PLAYERS 4












// ------------------------------------------------------ //
// ---------- Networking Structs & Definitions ---------- //
// ------------------------------------------------------ //

// ---- Message Types ---- //	(Format: 0x___)
#define MSG_CONNECT 0x001		// Connection request/confirmation
#define MSG_TSTEP   0x002		// Simulation Step request/reply. This data is shared in STRTGM, but we may want to sometimes retreive this information on it's own
#define MSG_REP     0x003		// Client state change request / server state replication
#define MSG_ID		0x004		// Carries the network ID for the actor being controlled by the recipient client. MIGHT BE DEPRECATED
#define MSG_SPAWN   0x005		// Client projectile Spawn / Server proxy link 
#define MSG_EXIT	0x006		// Client --> Server. Instructs server to stop
#define MSG_RPC		0x007		// Remote Procedure Call
#define MSG_STRTGM  0x008		// Client --> Server : Client is ready to start game | Server --> Client, game has started


// ---- RPCs ---- //	(Format: 0x1___)
#define RPC_TEST 0x1000			// For Debugging





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





// -------------------- Message structs -------------------- //

/* TODO: Modify this to hold only the data required for a certain RPC */

// MSG_SPAWN
struct NetworkSpawnData
{
private:
	char messageType = 's';
public:
	unsigned int simulationStep; // renamed from stateSequenceID
	unsigned int dummyActorID; // ID used to link dummy with authoritative client actor
	unsigned int networkedActorID;
};


// MSG_STRTGM
struct StartGameData
{
private:
	char messageType = MSG_STRTGM;

public:
	unsigned int simulationStep; // Simulation step the server was executing at time of message sending
};


// MSG_CONNECT (From Server)
struct ConnectAckData
{
private:
	char messageType = MSG_CONNECT;
public:
	unsigned int controlledActorID;
};


// MSG_ID (To Client)




