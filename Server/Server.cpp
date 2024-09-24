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
// I doubt any other kind of message will exceed the size of this type of message
char sendBuffer[1 + sizeof(unsigned int) + (MAX_ACTORS * sizeof(Actor))]{};
char* recvBuffer{};
sockaddr_in clientAddr;
int clientAddr_len = sizeof(clientAddr);
const unsigned int NUM_PACKETS_PER_CYCLE = 50;





// TODO: This should be in definitions
/* Exists so client IP's can be used as key's in a client to actor map. */
struct Client
{
	in_addr _in_addr;

	/* Actor controlled by this client */
	Actor* controlledActor = nullptr;

	/* Is the client ready to play the game */
	bool bIsReady = false;

	bool operator<(const Client& other) const
	{
		return _in_addr.S_un.S_addr < other._in_addr.S_un.S_addr;
	}
	bool operator==(const Client& other) const { return _in_addr.S_un.S_addr == other._in_addr.S_un.S_addr; }
};

#define MAX_CLIENTS 4
std::vector<Client> clients;
unsigned int numReadyClients = 0;


std::vector<Actor*> actors;



/* Stores the asteroids. Used to look for collision between player and asteroid */
std::set<Actor*> asteroids; 

/* Stores net data for all actors during the interval between the previous fixed update and the current update.
*  Actor's net data is stored here before being deleted. 
*  Upon next fixed update, all of these are used to generate a respective ActorNetData, which is used by clients to destroy their proxies
*/
std::vector<ActorNetData> actorsDestroyedThisUpdate = {};



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
	sock.init(true);
	clientAddr.sin_family = AF_INET;
	clientAddr.sin_port = htons(4242);
	bool bRunMainLoop = true;
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
		int numBytesRead;
		recvBuffer = sock.recvData(numBytesRead, clientAddr);
		if (numBytesRead > 0)
		{
			handleMessage(recvBuffer, numBytesRead);
		}
		
		if (bRunningStartGameTimer)
		{
			float currentSeconds = (getCurrentTime() - startTime).count(); // Seconds difference between start of epoch and now
			if ((int)currentSeconds > prevSecondsReached) // Truncating float
			{
				prevSecondsReached = currentSeconds;
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
		checkForCollisions(); // Potentially pushes actors onto `actorsDestroyedThisUpdate`


		// -- Fixed Frequency Update -- //
		if (secondsSinceLastUpdate >= updatePeriod) // ~20 times a second
		{
			secondsSinceLastUpdate -= updatePeriod;
			stateSequenceID++;

			// ---- Spawning Asteroids ---- //
			if ((stateSequenceID % 40) == 0)
			{	// Spawn Asteroid
				// std::cout << "TEST" << std::endl;
				Actor* asteroid = new Actor(
					Vector3D(0.f, 50.f, 25.f),
					Vector3D(0.f, -1.f, 0.f),
					ABI_Asteroid,
					false
				);
				actors.push_back(asteroid);
				asteroids.insert(asteroid);
			};
			//std::cout << actors.size() << std::endl;
			//std::cout << asteroids.size() << std::endl;

			// -- Sending Snapshot to Clients -- //
			if (clients.size() <= 0) return;

			// - Packing actor data - //
			sendBuffer[0] = MSG_REP;
			unsigned int offset = 1; // Number of bytes to reach memory to be filled

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
				std::cout << "clear" << std::endl;
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



void checkForCollisions()
{
	std::vector<Client>::iterator clientItr = clients.begin();
	while ( clientItr != clients.end() )
	{
		if (!clientItr->controlledActor) // Skips clients that aren't controlling anything
		{
			clientItr++;
			continue;
		}
		std::set<Actor*>::iterator asteroidItr = asteroids.begin();
		while (asteroidItr != asteroids.end())
		{
			// -- Checking for Collision between Client Actor & any Asteroid -- //
			Vector3D v1 = clientItr->controlledActor->getPosition();	// BUG: This can sometimes be pointing to end() while it's being dereferenced
			Vector3D v2 = (*asteroidItr)->getPosition(); // Wierd dereference bc we're storing pointers while using iterators
			v1.z = v2.z = 0.f; // Height is just cosmetic
			if ((v1 - v2).length() <= 20.f) 
			{
				// -- Updating actorsDestroyedThisUpdate -- //
				ActorNetData clientActorData = clientItr->controlledActor->toNetData();
				clientActorData.bIsDestroyed = true; // Queueing the actor to be destroyed later
				actorsDestroyedThisUpdate.push_back(clientActorData);

				ActorNetData asteroidNetData = (*asteroidItr)->toNetData();
				asteroidNetData.bIsDestroyed = true;
				actorsDestroyedThisUpdate.push_back(asteroidNetData);

				// -- Removing Destroyed Actors from Actor & Deleting them -- //
				auto controlledActorItr = std::find(actors.begin(), actors.end(), clientItr->controlledActor);
				actors.erase(controlledActorItr);
				delete clientItr->controlledActor;
				clientItr->controlledActor = nullptr;

				auto asteroidFromVector = std::find(actors.begin(), actors.end(), *asteroidItr);
				actors.erase(asteroidFromVector);
				delete (*asteroidItr);
				asteroidItr = asteroids.erase(asteroidItr); // Increments the iterator
				
				/*
				* We've destroyed the current client actor so it is pointless to continue to 
				* to compare more asteroids to it.
				* Thus, we leave the asteroid loop, and repeat the process for the next
				* actor-controlling client
				*/
				break;
			}
			asteroidItr++; // This occurs only if an asteroid hasn't been destroyed
		}//~ Asteroid loop
		clientItr++;
	}//~ Client loop
}


void readRecvBuffer()
{
	// ----  Receiving Data from Clients ----  //
	int recvBufferLen = 0;
	sockaddr_in recvAddr; // This is never used on purpose. Code smell
	for (int i = 0; i < NUM_PACKETS_PER_CYCLE; i++)
	{
		
		recvBuffer = sock.recvData(recvBufferLen, clientAddr);
		handleMessage(recvBuffer, recvBufferLen);
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

		// -- Creating Controlled Actor & Associating it with its Client -- //
		client.controlledActor = new Actor(Vector3D(0.f), Vector3D(0.f), ABI_PlayerCharacter);
		actors.push_back(client.controlledActor);
		clients.push_back(client);

		// -- Constructing & Sending Reply data -- //
		char buffer[sizeof(ConnectAckData)];
		ConnectAckData data;
		data.controlledActorID = client.controlledActor->getId();
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
	case MSG_SPAWN: // Client spawned an actor.
	{
		std::cout << "MSG_SPAWN" << std::endl;

		// -- Is sender a connected client? -- //
		auto clientItr = std::find(clients.begin(), clients.end(), client);
		if (clientItr == clients.end())
		{
			printf("MSG_SPAWN / Sender isn't a connected client. Doing nothing\n");
			return;
		}

		// -- Deformatting data & Spawning Actor -- // 
		NetworkSpawnData data;
		memcpy(&data, buffer + 1, sizeof(NetworkSpawnData));
		Actor* clientActor = std::find(clients.begin(), clients.end(), client)->controlledActor;
		Actor* projectile = new Actor(
			clientActor->getPosition() + (clientActor->getRotation() * 100.f),
			Vector3D(1.f, 0.f, 0.f),
			ABI_Projectile
		);
		actors.push_back(projectile);
		data.networkedActorID = projectile->getId();

		// -- Sending Reply to Client -- //
		memcpy(sendBuffer, &data, sizeof(NetworkSpawnData));
		sock.sendData(sendBuffer, sizeof(NetworkSpawnData), clientAddr);

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

	Client client;
	client._in_addr = clientAddr.sin_addr;
	switch (rpc.method)
	{
		case RPC_SPAWN:
		{
			// -- Spawning Actor -- // 
			Actor* clientActor = std::find(clients.begin(), clients.end(), client)->controlledActor;

			if (!clientActor)
			{
				printf("ERROR::handleRPC -- clientActor == nullptr\n");
				return;
			}

			Actor* projectile = new Actor(
				clientActor->getPosition() + (clientActor->getRotation() * 100.f),
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



