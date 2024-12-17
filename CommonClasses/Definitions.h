#pragma once



/*
* This file is a list of definitions commonly used by both the server and the client.
* Many of the structs here are meant to make the networking code cleaner when sending 
* and unpacking data.
* While the client and the server both use the same MSG types, they will sometimes
* send/receive different packages respectively. This type of struct will always
* be denoted with a snake-case suffix like "_server" or "_client".
* The suffix denotes which process sends the packet
*/




// --------------------------------------------------- //
// ---------- General Structs & Definitions ---------- //
// --------------------------------------------------- //

#define MAX_PLAYERS 4
#define MAX_ACTORS 64

/* Mask for actor id in a network ID.
*  0000 == Server owned
*/
#define CLIENT_NETWORK_ID_MASK !((1 << 24) - 1)



/* The input masks used to read the input bit-string sent by the client each frame, and read by the server */
#define INPUT_UP    0b00000001
#define INPUT_DOWN  0b00000010
#define INPUT_LEFT  0b00000100
#define INPUT_RIGHT 0b00001000
#define INPUT_SHOOT 0b00010000


// ----------------------------------------------- //
// ---------- Message & RPC Definitions ---------- //
// ----------------------------------------------- //

/* Message types. Often used in their respective structs. Here for readability. RPCs are also included */
#define MSG_CONNECT 0x001		// Connection request/confirmation
#define MSG_TSTEP   0x002		// Simulation Step request/reply. This data is shared in STRTGM, but we may want to sometimes retreive this information on it's own
#define MSG_REP     0x003		// Client state change request / server state replication
#define MSG_ID		0x004		// Carries the network ID for the actor being controlled by the recipient client. MIGHT BE DEPRECATED
#define MSG_SPAWN   0x005		// NOT USED
#define MSG_EXIT	0x006		// Client --> Server. Instructs server to stop
#define MSG_RPC		0x007		// Remote Procedure Call
#define MSG_STRTGM  0x008		// Client --> Server : Client is ready to start game | Server --> Client, game has started

#define RPC_SPAWN    0x001	



// --------------------------------------------------------- //
// -------------------- Message structs -------------------- //
// --------------------------------------------------------- //

// TODO: Implement and use. Server doesn't use this to pack itself. Inconsistent with rest of codebase
/* -- MSG_REP, Server Creates --
* Stores the net relevant data for each actor (ActorNetData) that needs to be replicated
*/
//struct GameState
//{
//private:
//	char messageType = MSG_REP;
//public:
//	/* Contiguous actor data. Read using the size of ActorNetData */
//	char data[MAX_ACTORS * sizeof(ActorNetData)];
//};

/* -- MSG_REP, Client Creates --
* Stores a frames input with that frames input request ID.
* The server uses the ID to know in which sequence the requests have been made, and process them
*/
struct ClientInputData
{
private:
	char messageType = MSG_REP;
public:
	/* Stores the input state for each possible input action, for a single client frame */
	char inputString;
	/* The request being made by the client */
	unsigned int inputRequestID;
	/* Delta time at the time of creation. Server calculates based on time the request is received, 
	*  and client uses local delta time.
	*  Maintained so application of input is properly scaled
	*/
	float deltaTime;
};




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
	int clientNetworkID;
};







// ----------------------------------------------------------------------- //
// -------------------- Remote Procedure Call structs -------------------- //
// ----------------------------------------------------------------------- //
/*
* The Header for all RPC messages
*/
struct RemoteProcedureCall
{
private:
	char messageType = MSG_RPC;

public:
	/* Which RPC will be called*/
	char method;

	/* Simulation Step the RPC is being called during */
	unsigned int simulationStep;

	/* Seconds after the last fixed frequency update completed */
	float secondsSinceLastUpdate;

	/* Dynamic to allow different sizes of RPC message bodies to be used without wasting space */
	char message[20];
};