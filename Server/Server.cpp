#include <chrono>

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



/*
* Exists so client IP's can be used as key's in a client to actor map.
*/
struct IpAddress
{
	in_addr _in_addr;

	bool operator<(const IpAddress& other) const
	{
		return _in_addr.S_un.S_addr < other._in_addr.S_un.S_addr;
	}
};




UDPSocket sock;
// I doubt any other kind of message will exceed the size of this type of message
char sendBuffer[1 + sizeof(unsigned int) + (MAX_ACTORS * sizeof(Actor))]{};
char* recvBuffer{};
sockaddr_in clientAddr;
int clientAddr_len = sizeof(clientAddr);





#define MAX_CLIENTS 4
std::map<IpAddress, Actor*> clients;
unsigned int numClients = 0;

Actor* actors[MAX_ACTORS]{}; // Actors created and replicated
unsigned int numActors = 0;





bool bRunMainLoop = true;

float updatePeriod = 1.f / 20.f; // Verbose to allow easy editing. Should be properly declared later // 20.f
float deltaTime = 0.f;
unsigned int stateSequenceID = 0;




/*
* KEY: Simulation step each set of actors are to begin being simulated
* VALUE: Set of actors to be simulated upon reaching the correct simulation step
*/
std::map<unsigned int, std::set<Actor*>> preSpawnedActors;



// TODO: Implement Destroy Actor
void moveActors(float deltaTime);

/*
buffer : The message as a string of data
bufferLen : number of char's in the buffer
*/
void handleMessage(char* buffer, unsigned int bufferLen);


// ---- SERVER ---- //
int main()
{
	sock.init(true);
	clientAddr.sin_family = AF_INET;
	clientAddr.sin_port = htons(4242);


	std::chrono::steady_clock::time_point start = std::chrono::high_resolution_clock::now();
	std::chrono::steady_clock::time_point end;


	float elapsedTimeSinceUpdate = 0.f;
	while (bRunMainLoop)
	{
		// -- Time Management -- //	
		end = start;
		start = std::chrono::high_resolution_clock::now();
		deltaTime = (start - end).count() / 1000000000.f;	// nanoseconds to seconds
		elapsedTimeSinceUpdate += deltaTime;

		// -- Updating Actors -- //
		moveActors(deltaTime);

		
		// BUG: There are two actors being created when we create an actor here

		// -- Checking for spawn/respawn of player characters -- //
		auto iter = clients.begin();
		while (iter != clients.end())
		{
			if (iter->second == nullptr)
			{
				printf("Creating client actor\n");

				iter->second = new Actor(Vector3D(0.f), Vector3D(0.f), ABI_PlayerCharacter);
				actors[numActors] = iter->second;
				numActors++;
				//iter->second = createActor(position, rotation);
				//iter->second->InitializeModel("C:/Users/User/source/repos/Multiplayer-Asteroids/CommonClasses/FBX/Gear/Gear1.fbx");
				// QUESTION: Why would creating an actor in this way result in a nullptr error when trying to replicate
				//iter->second = new Actor(position, rotation);
				//numActors++;
				//actors[numActors] = iter->second;

				// -- Sending actor ID to client -- //
				sendBuffer[0] = MSG_ID;
				unsigned int id = iter->second->getId();
				memcpy(sendBuffer + 1, &id, sizeof(unsigned int));
				/* QUESTION: Why would memcpy ing using the below line result in an exception being thrown ??? */
				//memcpy(sendBuffer + 1, (void*)clients[iter->first]->getId(), sizeof(unsigned int));

				clientAddr.sin_addr = iter->first._in_addr;
				clientAddr.sin_family = AF_INET;
				clientAddr.sin_port = htons(4242);
				sock.sendData(sendBuffer, 1 + sizeof(unsigned int), clientAddr);
			}
			iter++;
		}


		// -- Handling Client Messages -- //
		int numBytesRead;
		recvBuffer = sock.recvData(numBytesRead, clientAddr);
		if (numBytesRead > 0) // Received Anything?
		{
			handleMessage(recvBuffer, numBytesRead);
		}


		// -- Fixed Frequency Update -- //
		if (elapsedTimeSinceUpdate >= updatePeriod) // ~20 times a second
		{

			elapsedTimeSinceUpdate -= updatePeriod;
			stateSequenceID++;

			// std::cout << "Fixed update " << stateSequenceID << std::endl;
		
			// -- Sending Snapshot to Clients -- //
			if (numClients <= 0) continue;

			// - Packing actor data - //
			sendBuffer[0] = MSG_REP;
			unsigned int offset = 1; // Number of bytes to reach memory to be filled

			memcpy(sendBuffer + 1, &stateSequenceID, sizeof(unsigned int));
			offset += sizeof(unsigned int);

			for (unsigned int i = 0; i < numActors; i++)
			{
				// Actor::serialize(tempBuffer, actors[i]);
				ActorNetData data = actors[i]->toNetData();

				// std::cout << "SENDING: " << data.Position.toString() << std::endl;
			
				memcpy(sendBuffer + 1 + sizeof(unsigned int) + (i * sizeof(ActorNetData)), &data, sizeof(ActorNetData));
				offset += sizeof(ActorNetData);
			}
			// - Sending Actor data - //
			for (auto it = clients.begin(); it != clients.end(); ++it)
			{
				// offset is now the length of the buffer in bytes
				clientAddr.sin_addr = it->first._in_addr;
				sock.sendData(sendBuffer, 1 + sizeof(unsigned int) + (numActors * sizeof(ActorNetData)), clientAddr);
			}
		}//~ Fixed Update

	};//~ Main Loop

	WSACleanup();

	// Prevents window from closing too quickly. Good so I can see print statements
	char temp[200];
	std::cin >> temp;

	return 0;
};






void moveActors(float deltaTime)
{
	for (unsigned int i = 0; i < numActors; i++)
	{
		Actor* actor = actors[i];
		actor->addToPosition(actor->getMoveDirection() * actor->getMoveSpeed() * deltaTime);


		// std::cout << "moveActors / actor: " << i << " / " << actors[i]->getPosition().toString() << std::endl;

		// TODO: Implement propery approach to limiting actor movement. Different types of actors will handle the edge of the screen 
		// -- X-Boundary Checking -- //
		if (actor->getPosition().x > 62.f) // I have no idea why this value is edge of the screen
		{
			actor->setPosition(Vector3D(62.f, actor->getPosition().y, actor->getPosition().z));
		}
		else if (actor->getPosition().x < -62.f)
		{
			actor->setPosition(Vector3D(-62.f, actor->getPosition().y, actor->getPosition().z));
		}

		// -- Y-Boundary Checking -- //
		if (actor->getPosition().y > 50.f)
		{
			actor->setPosition(Vector3D(actor->getPosition().x, 50.f, actor->getPosition().z));
		}
		else if (actor->getPosition().y < -50.f)
		{
			actor->setPosition(Vector3D(actor->getPosition().x, -50.f, actor->getPosition().z));
		}
	}
}


void handleMessage(char* buffer, unsigned int bufferLen)
{
	IpAddress ip;
	ip._in_addr = clientAddr.sin_addr;
	switch (recvBuffer[0]) // Instruction Received
	{
	case MSG_CONNECT: // Connect. Client wants to be added to list of connected clients
	{
		std::cout << "handleMessage / MSG_CONNECT" << std::endl;
		clients.insert({ ip, nullptr });
		numClients++;

		// - replying to message - //
		sendBuffer[0] = MSG_CONNECT;
		sock.sendData(sendBuffer, 1, clientAddr);
		break;
	}
	case MSG_TSTEP: // Request for current state sequence id or "Timestep"
	{
		std::cout << "handleMessage / MSG_TSTEP / Sending state sequence ID: " << stateSequenceID << std::endl;
		sendBuffer[0] = MSG_TSTEP;
		memcpy(sendBuffer + 1, &stateSequenceID, sizeof(unsigned int));
		sock.sendData(sendBuffer, 1 + sizeof(unsigned int), clientAddr);
	}
	case MSG_REP: // Client sending actor movement update
	{
		// std::cout << "handleMessage / MSG_REP" << std::endl;
		// Receives client inputs and uses them to update the state of that client's playerActor
		if (clients.count(ip) == 0)
		{
			std::cout << "handleMessage / MSG_REP : received replication of input from unknown client. Ignoring";
			break;
		}
		if (clients[ip] == nullptr)
		{
			// The client attempting to replicate has no actor to move
			break;
		}
		ActorNetData netData;
		memcpy(&netData, recvBuffer+1, sizeof(netData));
		Actor* actor = clients[ip];
		actor->setMoveDirection(netData.moveDirection);
		break;
	}
	case MSG_SPAWN: // Client spawned an actor.
	{
		std::cout << "handleMessage / MSG_SPAWN" << std::endl;

		// -- Deformatting data & Spawning Actor -- // 
		NetworkSpawnData data;
		memcpy(&data, recvBuffer, sizeof(NetworkSpawnData));

		Actor* clientActor = clients[ip];
		Actor* projectile = new Actor(
			clientActor->getPosition() + (clientActor->getRotation() * 100.f),
			Vector3D(1.f, 0.f, 0.f),
			ABI_Projectile
		);
		actors[numActors] = projectile;
		numActors++;
		data.networkedActorID = projectile->getId();

		// -- Sending Reply to Client -- //
		char replyBuffer[sizeof(NetworkSpawnData)];
		sock.sendData(replyBuffer, sizeof(NetworkSpawnData), clientAddr);

		break;
	}
	case MSG_EXIT: 
	{
		std::cout << "handleMessage / MSG_EXIT" << std::endl;
		bRunMainLoop = false;
		break;
	}
	}
}