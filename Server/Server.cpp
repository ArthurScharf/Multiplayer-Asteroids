#include <chrono>
#include <functional>
#include <iostream>
#include <map>
#include <set>
#include <tchar.h> // _T
#include <thread>
#include <WinSock2.h>
#include <WS2tcpip.h> // inetPtons

#include "../CommonClasses/Actor.h"
#include "../CommonClasses/Definitions.h"
#include "../CommonClasses/Vector3D.h"
#include "../CommonClasses/UDPSocket.h"






// Linking (What does this have to do with linking????
#pragma comment(lib, "ws2_32.lib")




UDPSocket sock;
/*
* The sending and receiving arrays for the program. These are declared here to be on the `data` section of the processes memory
* because they are allocated for the programs lifetime
* 
* I doubt any other kind of message will exceed the size of this type of message
*/
char sendBuffer[SIZE_OF_NETWORK_BUFFER]{}; 
char recvBuffer[SIZE_OF_NETWORK_BUFFER]{};
// Stores the internet address for the currently being processed client
sockaddr_in clientAddr;
int clientAddr_len = sizeof(clientAddr);
// A constant for processing packets per a cycle of the main loop. Value is guessed-and-checked
const unsigned int NUM_PACKETS_PER_CYCLE = 10;





// TODO: This should be in definitions. Client.cpp should use this. Should be renamed to clientNetworkData
/* Exists so client IP's can be used as key's in a client to actor map. */
struct Client
{
	in_addr _in_addr;

	/* A byte used to ID ownership of actors over the network.
	* The most significant byte of each actor ID is set to reflect it's ownership.
	* a value of 0 indicates the server owns the actor
	*/
	char clientNetworkID;

	/*
	* Predicted ID for the next actor created for a client.
	* Each client must be able to predict the ID for actors it's spawning predictively.
	* If all the clients shared the same pool of Network IDs, there would be no way
	* for any client to predict an ID without there being a possibility of
	* another client having already predicted that there spawned actor would use the same ID.
	* Thus we split the ID's into 4 different pools using network ownership masks (see Definitions.h),
	* 
	* TODO: As of time of writing, it seems EXTREMELY unlikely that we'll ever use all of our IDs.
	*       While this is bad to ignore, we're just going to assume it's never reached
	*/
	unsigned int nextActorNetworkID = 0;


	/* Actor controlled by this client */
	Actor* controlledActor = nullptr;

	/* Is the client ready to play the game */
	bool bIsReady = false;

	bool operator<(const Client& other) const
	{
		return _in_addr.S_un.S_addr < other._in_addr.S_un.S_addr;
	}
	bool operator==(const Client& other) const { return _in_addr.S_un.S_addr == other._in_addr.S_un.S_addr; }


	void setClientNetworkID(char _clientNetworkID)
	{
		// Is the input setting non-client-network-ID-bits?
		if (~(((clientNetworkID << 24) & CLIENT_NETWORK_ID_MASK)) > 0)
		{
			printf("ERROR - Client::setClientNetworkID - passed network ID was too large. Would have overwritten non-client-network-bits");
			return;
		}
		clientNetworkID = _clientNetworkID;
	}

	// returns the combined client id and next actor id. increments the next actor ID by 1
	unsigned int getNextCombinedNetworkID()
	{
		unsigned int out_id = clientNetworkID | nextActorNetworkID;
		nextActorNetworkID++;
		return out_id;
	}
};

#define MAX_CLIENTS 4
std::vector<Client> clients;
std::vector<int> unclaimedClientNetworkIDs = { 0b00000001, 0b00000010, 0b00000011, 0b00000100 };
unsigned int numReadyClients = 0;



// -- Actors -- //

/* Used to generate server owned actors */
unsigned int nextActorNetworkID_Server = 0;

std::vector<Actor*> actors;

/* Stores net data for all actors during the interval between the previous fixed update and the current update.
*  Actor's net data is stored here before being deleted. 
*  Upon next fixed update, all of these are used to generate a respective ActorNetData, which is used by clients to destroy their proxies
*/
std::vector<ActorNetData> actorsDestroyedThisUpdate = {};



// -- Time -- //
using Time = std::chrono::steady_clock;
//using ms = std::chrono::milliseconds;
using float_sec = std::chrono::duration<float>;
using float_time_point = std::chrono::time_point<Time, float_sec>;

float updatePeriod = 1.f / 20.f; // Verbose to allow easy editing. Should be properly declared later // 20.f
float deltaTime = 0.f;
float secondsSinceLastUpdate = 0.f;
unsigned int stateSequenceID = 0;

float_time_point startTime; // Time that begin-game-countdown was started
bool bRunningStartGameTimer = false;
int prevSecondsReached = 0; // This being here suggests we should encapsulate. Next time




/* Stores remote procedure calls yet to be executed */
std::map<unsigned int, std::vector<RemoteProcedureCall>> remoteProcedureCalls;




enum ServerState
{
	SS_WaitingForConnections,
	SS_PlayingGame,
	SS_Exit
};


ServerState currentState = SS_WaitingForConnections;





void waitingForConnectionsLoop();

void playingGameLoop();

void moveActors(float deltaTime);

void checkForCollisions();

void readRecvBuffer();

void handleMessage(char* buffer, unsigned int bufferLen);

void handleRPC(RemoteProcedureCall rpc);

float_time_point getCurrentTime();




// ---- SERVER ---- //
int main()
{
	// -- Winsock Initialization -- //
	sock.init(true);
	clientAddr.sin_family = AF_INET;
	clientAddr.sin_port = htons(4242);
	
	
	// -- Program Specific Initialization -- //
	bool bRunMainLoop = true;


	// -- Main Loop -- //
	while (bRunMainLoop)
	{
		switch (currentState)
		{
		case SS_WaitingForConnections:
		{
			printf("Waiting for player Connections\n");
			waitingForConnectionsLoop();
			break;
		}
		case SS_PlayingGame:
		{
			//actors.push_back(new Actor(
			//	nextActorNetworkID_Server++,
			//	Vector3D(40.f, 25.f, 0),
			//	Vector3D(),
			//	EActorBlueprintID::ABI_Asteroid
			//));
			printf("Beginning Game\n");
			playingGameLoop();
			break;
		}
		case SS_Exit:
		{
			printf("Exiting\n");
			bRunMainLoop = false;
			break;
		}
		}//~ Switch
	}

	WSACleanup();

	// Prevents window from closing too quickly. Good so I can see print statements
	char temp[200];
	std::cin >> temp;

	return 0;
};



void waitingForConnectionsLoop()
{
	while (currentState == SS_WaitingForConnections)
	{
		// -- Message Handling -- //
		int numBytesRead;
		sock.recvData(recvBuffer, SIZE_OF_NETWORK_BUFFER, numBytesRead, clientAddr);
		if (numBytesRead > 0)
		{
			handleMessage(recvBuffer, numBytesRead);
		}
		
		// -- Start Game Timer -- //
		if (bRunningStartGameTimer)
		{
			float currentSeconds = (getCurrentTime() - startTime).count(); // Seconds difference between start of epoch and now
			if ((int)currentSeconds > prevSecondsReached) // Truncating float
			{
				prevSecondsReached = static_cast<int>(currentSeconds); // truncation desired
				printf("	%d\n", 2 - (int)currentSeconds);
			}

			// -- Attempt to start match -- //
			if (currentSeconds >= 2.f)
			{	
				bRunningStartGameTimer = false; // Defensive programming practice. Isn't necassary

				// -- Notifying clients -- //
				StartGameData data;
				data.simulationStep = stateSequenceID;
				memcpy(sendBuffer, &data, sizeof(StartGameData));
				for (auto itr = clients.begin(); itr != clients.end(); itr++)
				{
					clientAddr.sin_addr = itr->_in_addr;
					sock.sendData(sendBuffer, sizeof(StartGameData), clientAddr);
				}

				// -- Changing States -- //
				currentState = SS_PlayingGame;
			}
		}

	}//~ Loop
}


void playingGameLoop()
{
	std::chrono::steady_clock::time_point start = std::chrono::high_resolution_clock::now();
	std::chrono::steady_clock::time_point end;

	while (currentState == SS_PlayingGame)
	{
		// printf("%d / %d\n", actorsDestroyedThisUpdate.size(), actors.size());


		// ---- Time Management ---- //	
		end = start;
		start = std::chrono::high_resolution_clock::now();
		deltaTime = (start - end).count() / 1000000000.f;	// nanoseconds to seconds
		secondsSinceLastUpdate += deltaTime;


		// ---- Receiving Messages ---- //
		readRecvBuffer(); // Reads NUM_PACKETS_PER_CYCLE worth of messages
		


		// ---- Handling RPCs ---- //
		if (remoteProcedureCalls.count(stateSequenceID) != 0) // Counts number of elements with specified key
		{
			// -- Getting RPC's for this update -- //
			
			std::cout << stateSequenceID << "/ Handling RPCs / # RPCS to Call: " << remoteProcedureCalls[stateSequenceID].size() << std::endl;

			auto rpcItr = remoteProcedureCalls[stateSequenceID].begin();
			while (rpcItr != remoteProcedureCalls[stateSequenceID].end())
			{
				handleRPC(*rpcItr);
				rpcItr = remoteProcedureCalls[stateSequenceID].erase(rpcItr); // iterator is set to next value

				//if (rpcItr->secondsSinceLastUpdate >= secondsSinceLastUpdate)
				//{
				//	handleRPC(*rpcItr);
				//	rpcItr = toCall->erase(rpcItr); // iterator is set to next value
				//	continue;
				//}
				//rpcItr++;
			}
			// Removing the RPC list for this state if it's empty
			if (remoteProcedureCalls[stateSequenceID].empty())
			{
				remoteProcedureCalls.erase(stateSequenceID);
			}
		}//~ handling RPCs


		// -- Updating Actors -- //
		moveActors(deltaTime);
		// checkForCollisions(); // Potentially pushes actors onto `actorsDestroyedThisUpdate`


		// -- Fixed Frequency Update -- //
		if (secondsSinceLastUpdate >= updatePeriod) // ~20 times a second
		{
			// std::cout << "FFU / numActors: " << actors.size() << std::endl;


			secondsSinceLastUpdate -= updatePeriod;
			stateSequenceID++;

			// ---- Spawning Asteroids ---- //
			
			//if ((stateSequenceID % 40) == 0)
			//{	// Spawn Asteroid
			//	// std::cout << "TEST" << std::endl;
			//	Actor* asteroid = new Actor(
			//		10 + nextActorNetworkID_Server++,
			//		Vector3D(0.f, 50.f, 25.f),
			//		Vector3D(0.f, -1.f, 0.f),
			//		ABI_Asteroid,
			//		false
			//	);
			//	actors.push_back(asteroid);
			//};
			

			// -- Sending Snapshot to Clients -- //
			if (clients.size() <= 0) return;

			// - Packing actor data - //
			sendBuffer[0] = MSG_REP;
			unsigned int offset = 1; // Number of bytes to reach memory to be filled

			// printf("FFU %d -- Actor Size: %d\n", stateSequenceID, actors.size());

			memcpy(sendBuffer + 1, &stateSequenceID, sizeof(unsigned int));
			offset += sizeof(unsigned int);

			for (unsigned int i = 0; i < actors.size(); i++) // Iterating Actors
			{
				ActorNetData data = actors[i]->toNetData();

				memcpy(sendBuffer + 1 + sizeof(unsigned int) + (i * sizeof(ActorNetData)), &data, sizeof(ActorNetData));
				offset += sizeof(ActorNetData);
			}
			for (unsigned int i = 0; i < actorsDestroyedThisUpdate.size(); i++) // Iterating data from destroyed actors
			{
				memcpy(sendBuffer + offset, &actorsDestroyedThisUpdate[i], sizeof(ActorNetData));
				offset += sizeof(ActorNetData);
			}

			// - Sending Actor data - //
			for (auto it = clients.begin(); it != clients.end(); ++it)
			{
				// offset is now the length of the buffer in bytes
				clientAddr.sin_addr = it->_in_addr;
				sock.sendData(sendBuffer, offset, clientAddr);
			}

			// -- Clearing Actors Destroyed from This Update -- //
			if (actorsDestroyedThisUpdate.size() > 0)
			{
				actorsDestroyedThisUpdate.clear();
			}
		}//~ Fixed Update

	};//~ Main Loop
}


// TODO: BUG: since I'm modifying the array while iterating (doing so with indices), this will create issues
void moveActors(float deltaTime)
{
	for (unsigned int i = 0; i < actors.size(); i++)
	{
		Actor* actor = actors[i];
		actor->addToPosition(actor->getMoveDirection() * actor->getMoveSpeed() * deltaTime);

		// std::cout << "moveActors / actor: " << i << " / " << actors[i]->getPosition().toString() << std::endl;
		
		bool bDestroyActor = false;

		// TODO: Implement propery approach to limiting actor movement. Different types of actors will handle the edge of the screen 
		// -- X-Boundary Checking -- //
		if (actor->getPosition().x > 62.f) // I have no idea why this value is edge of the screen
		{
			actor->setPosition(Vector3D(62.f, actor->getPosition().y, actor->getPosition().z));
			bDestroyActor = true;
		}
		else if (actor->getPosition().x < -62.f)
		{
			actor->setPosition(Vector3D(-62.f, actor->getPosition().y, actor->getPosition().z));
			bDestroyActor = true;
		}

		// -- Y-Boundary Checking -- //
		if (actor->getPosition().y > 50.f)
		{
			actor->setPosition(Vector3D(actor->getPosition().x, 50.f, actor->getPosition().z));
			bDestroyActor = true;
		}
		else if (actor->getPosition().y < -50.f)
		{
			actor->setPosition(Vector3D(actor->getPosition().x, -50.f, actor->getPosition().z));
			bDestroyActor = true;
		}

		// -- Destroying Actor if out of bounds -- //
		if (bDestroyActor)
		{
			actors.erase(actors.begin() + i);		
			ActorNetData data = actor->toNetData();
			data.bIsDestroyed = true;
			actorsDestroyedThisUpdate.push_back(data);
			delete actor;
		}
	}
}


// This method is still hot garbage. It doesn't check for collisions between anything other than asteroids and playerActors
void checkForCollisions()
{
	
	//-- Sort into temporary containers		NOTE: It would be faster if the actors were pre-sorted, of if there was a better way to sort them
	std::vector<Actor*> nonAsteroids;
	std::vector<Actor*> asteroids;
	for (Actor* actorPtr : actors)
	{
		if (actorPtr->getBlueprintID() == EActorBlueprintID::ABI_Asteroid)
		{
			asteroids.push_back(actorPtr);
		}
		else
		{
			nonAsteroids.push_back(actorPtr);
		}
	}


	//-- Brute force search for collisions
	std::vector<Actor*>::iterator asteroidItr;
	std::vector<Actor*>::iterator nonAsteroidItr;
	for (asteroidItr = asteroids.begin(); asteroidItr != asteroids.end();)
	{
		for (nonAsteroidItr = nonAsteroids.begin(); nonAsteroidItr != nonAsteroids.end();)
		{
			// If the length of the difference vector of the positions of the two actors is less than the sum of their collision radii, then they have collided
			if (((*asteroidItr)->getPosition() - (*nonAsteroidItr)->getPosition()).length() >= (*asteroidItr)->getCollisionRadius() + (*nonAsteroidItr)->getCollisionRadius())
			{
				// Creating net data for the two destroyed actors and placing this data in the container
				ActorNetData data = (*asteroidItr)->toNetData();
				data.bIsDestroyed = true;
				actorsDestroyedThisUpdate.push_back(data);
				data = (*nonAsteroidItr)->toNetData();
				data.bIsDestroyed = true;
				actorsDestroyedThisUpdate.push_back(data);


				// If client controlled. Handling client destruction. 
				// NOTE: Would be better if actors had a pointer to an owning client. Wouldn't have to search clients. Since num clients is small, this is ok
				for (Client &client : clients)
				{
					// If actor that collided was owned, disown it
					if (client.controlledActor = (*nonAsteroidItr))
					{
						client.controlledActor = nullptr;
						break;
					}
				}

				// destroying the actors
				delete (*asteroidItr);
				delete (*nonAsteroidItr);

				// Deleting the actors from their respective temporary containers
				asteroidItr    = asteroids.erase(asteroidItr);
				nonAsteroidItr = nonAsteroids.erase(nonAsteroidItr);
			}
			else
			{
				asteroidItr++;
				nonAsteroidItr++;
			}
		}//~ inner collision loop
	}//~ outer collision loop
}


void readRecvBuffer()
{
	// ----  Receiving Data from Clients ----  //
	int numBytesRead = 0;
	sockaddr_in recvAddr; // This is never used on purpose. Code smell
	for (int i = 0; i < NUM_PACKETS_PER_CYCLE; i++)
	{
		sock.recvData(recvBuffer, SIZE_OF_NETWORK_BUFFER, numBytesRead, clientAddr);
		if (numBytesRead != 0)
			handleMessage(recvBuffer, numBytesRead);
	}
}


void handleMessage(char* buffer, unsigned int bufferLen)
{
	Client client;
	client._in_addr = clientAddr.sin_addr;
	switch (recvBuffer[0]) // Instruction Received
	{
	case MSG_CONNECT:
	{
		char ipStr[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &client._in_addr, ipStr, INET_ADDRSTRLEN);
		printf("MSG_CONNECT -- Client IP: %s\n", ipStr);

		if (clients.size() >= MAX_CLIENTS)
		{
			printf("  Maximum number of clients already connected\n");
			break;
		}

		// -- Creating Controlled Actor & Associating it with its Client -- //
		client.controlledActor = new Actor(
			client.getNextCombinedNetworkID(),
			Vector3D(0.f), 
			Vector3D(0.f), 
			ABI_PlayerCharacter, 
			false
		);
		client.clientNetworkID = unclaimedClientNetworkIDs[0];
		clients.push_back(client); // Pushes a copy of local client onto `clients`.
		unclaimedClientNetworkIDs.erase(unclaimedClientNetworkIDs.begin());
		actors.push_back(client.controlledActor);


		// -- Constructing & Sending Reply data -- //
		char buffer[sizeof(ConnectAckData)];
		ConnectAckData data;
		data.controlledActorID = client.controlledActor->getId();
		data.clientNetworkID = client.clientNetworkID;
		std::cout << "client net id: " << data.clientNetworkID << std::endl;
		memcpy(buffer, &data, sizeof(ConnectAckData));
		sock.sendData(buffer, sizeof(ConnectAckData), clientAddr);
		break;
	}
	case MSG_STRTGM:
	{
		char ipStr[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &clientAddr.sin_addr, ipStr, INET_ADDRSTRLEN);
		printf("handleMessage / MSG_STRTGM from: %s\n", ipStr);


		auto clientItr = std::find(clients.begin(), clients.end(), client);
		// -- Is the sender a connected client? OR Is Client Already Ready? -- //
		if (clientItr == clients.end())
		{
			std::cout << "Sender isn't a connected client" << std::endl;
			return;
		}
		
		// -- Toggling Ready Status -- //
		numReadyClients += clientItr->bIsReady ? -1 : 1;
		clientItr->bIsReady = !clientItr->bIsReady;
		printf("Number of ready clients: %d\n", numReadyClients);

		// -- Start or Stop Timer -- //
		if (numReadyClients == clients.size() && !bRunningStartGameTimer)
		{
			bRunningStartGameTimer = true;
			startTime = getCurrentTime();
			printf("Game starts in...\n");
		}
		else
		{
			bRunningStartGameTimer = false;
			prevSecondsReached = 0;
			printf("Not all clients are ready. Stopping timer...");
		}
		break;
	}
	case MSG_TSTEP: // Request for current state sequence id or "Timestep"
	{
		std::cout << "MSG_TSTEP / Sending state sequence ID: " << stateSequenceID << std::endl;
		sendBuffer[0] = MSG_TSTEP;
		memcpy(sendBuffer + 1, &stateSequenceID, sizeof(unsigned int));
		sock.sendData(sendBuffer, 1 + sizeof(unsigned int), clientAddr);
		break;
	}
	case MSG_REP: // Client sending actor movement update
	{
		// std::cout << "handleMessage / MSG_REP" << std::endl;
		// Receives client inputs and uses them to update the state of that client's playerActor
		if (clients.size() == 0)
		{
			std::cout << "MSG_REP : received replication of input from unknown client. Ignoring";
			break;
		}
		auto clientItr = std::find(clients.begin(), clients.end(), client); // Silly need to switch to an iterator since the client we created earlier isn't the same as the one that is stored
		if (!clientItr->controlledActor)
		{		// The client attempting to replicate has no actor to move
			break;
		}
		ActorNetData netData;
		memcpy(&netData, buffer + 1, sizeof(netData));
		clientItr->controlledActor->setMoveDirection(netData.moveDirection);
		break;
	}
	case MSG_RPC:
	{
		std::cout << "MSG_RPC " << std::endl;
		RemoteProcedureCall rpc;
		memcpy(&rpc, buffer, sizeof(RemoteProcedureCall));
		std::cout << "        " << "stateSequenceID: " << stateSequenceID << " | rpc.simulationStep: " << rpc.simulationStep << std::endl;

		if (rpc.simulationStep < stateSequenceID)
		{ // This RPC call is too late to be simulated
			printf("ERROR::handleMessage -- RPC call arrived too late\n called at step: %d\n current step: %d", rpc.simulationStep, stateSequenceID);
			return;
		}
		remoteProcedureCalls[rpc.simulationStep].push_back(rpc);

		std::cout << "MSG_RPC -- " << remoteProcedureCalls.size() << std::endl;
		break;
	}
	case MSG_EXIT: 
	{
		std::cout << "MSG_EXIT" << std::endl;
		currentState = SS_Exit;
		break;
	}
	}
}


void handleRPC(RemoteProcedureCall rpc)
{
	std::cout << "handleRPC" << std::endl;

	// -- Finding Client -- //
	Client* clientPtr = nullptr;
	for (int i = 0; i < clients.size(); i++)
	{
		if (clients[i]._in_addr.S_un.S_addr == clientAddr.sin_addr.S_un.S_addr) // Does this work?
		{
			clientPtr = &clients[i];
		}
	}
	if (clientPtr == nullptr) 
	{  // Client that sent the RPC not found
		printf("WARNING::handleRPC -- clientAddr.sin_addr.S_un.S_addr doesn't correspond to any connected clients");
		return;
	}

	switch (rpc.method)
	{
		case RPC_SPAWN:
		{
			printf("handleRPC -- Spawn Projectile\n");

			/* --Spawning Projectile-- */
			Actor* projectile = new Actor(
				clientPtr->getNextCombinedNetworkID(),
				clientPtr->controlledActor->getPosition() + (clientPtr->controlledActor->getRotation() * 10.f),
				Vector3D(1.f, 0.f, 0.f),
				ABI_Projectile,
				false
			);
			actors.push_back(projectile);
			break;
		}
	}
}


float_time_point getCurrentTime()
{
	return Time::now(); // Implicitly casting
}


