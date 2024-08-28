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
		// -- Time Management -- //	
		end = start;
		start = std::chrono::high_resolution_clock::now();
		deltaTime = (start - end).count() / 1000000000.f;	// nanoseconds to seconds
		secondsSinceLastUpdate += deltaTime;


		int numBytesRead;
		recvBuffer = sock.recvData(numBytesRead, clientAddr);
		if (numBytesRead > 0)
		{
			handleMessage(recvBuffer, numBytesRead);
		}


		// -- Handling RPCs -- //	TODO: This process isn't very efficient
		std::vector<RemoteProcedureCall> uncalledRPCs;
		if (remoteProcedureCalls.count(stateSequenceID) != 0)
		{	// We have procedure calls that must be handled

			std::vector<RemoteProcedureCall> toCall = remoteProcedureCalls[stateSequenceID];

			for (int i = 0; i < toCall.size(); i++)
			{
				if (toCall[i].secondsSinceLastUpdate >= secondsSinceLastUpdate)
				{
					handleRPC(toCall[i]);
				}
				else
				{
					uncalledRPCs.push_back(toCall[i]);
				}
			}

			// -- Retaining uncalled RPCS -- //
			toCall.empty();

			if (uncalledRPCs.size() != 0)
			{
				remoteProcedureCalls[stateSequenceID] = uncalledRPCs;
			}
			else
			{
				remoteProcedureCalls.erase(stateSequenceID); // This case avoids rechecking
			}
		}//~ Handling RPCs


		// -- Updating Actors -- //
		moveActors(deltaTime);

		// -- Fixed Frequency Update -- //
		if (secondsSinceLastUpdate >= updatePeriod) // ~20 times a second
		{

			secondsSinceLastUpdate -= updatePeriod;
			stateSequenceID++;

			// std::cout << "Fixed update " << stateSequenceID << std::endl;

			// -- Sending Snapshot to Clients -- //
			if (clients.size() <= 0) continue;

			// - Packing actor data - //
			sendBuffer[0] = MSG_REP;
			unsigned int offset = 1; // Number of bytes to reach memory to be filled

			memcpy(sendBuffer + 1, &stateSequenceID, sizeof(unsigned int));
			offset += sizeof(unsigned int);

			for (unsigned int i = 0; i < actors.size(); i++)
			{
				ActorNetData data = actors[i]->toNetData();

				// std::cout << "SENDING: " << data.Position.toString() << std::endl;

				memcpy(sendBuffer + 1 + sizeof(unsigned int) + (i * sizeof(ActorNetData)), &data, sizeof(ActorNetData));
				offset += sizeof(ActorNetData);
			}
			// - Sending Actor data - //
			for (auto it = clients.begin(); it != clients.end(); ++it)
			{
				// offset is now the length of the buffer in bytes
				clientAddr.sin_addr = it->_in_addr;
				sock.sendData(sendBuffer, 1 + sizeof(unsigned int) + (actors.size() * sizeof(ActorNetData)), clientAddr);
			}
		}//~ Fixed Update

	};//~ Main Loop
}




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
			delete actor;
		}

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
		memcpy(&rpc, buffer + 1, sizeof(RemoteProcedureCall));
		std::cout << "        " << "stateSequenceID: " << stateSequenceID << " | rpc.simulationStep" << rpc.simulationStep << std::endl;

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
	switch (rpc.method)
	{
	case RPC_TEST:
	{
		printf("handleRPC / MSG_TEST -- %s", rpc.message);
		break;
	}
	}
}


float_time_point getCurrentTime()
{
	return Time::now(); // Implicitly casting
}



