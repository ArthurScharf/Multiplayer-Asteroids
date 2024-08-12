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






float updatePeriod = 1.f / 20.f; // Verbose to allow easy editing. Should be properly declared later // 20.f
float deltaTime = 0.f;

unsigned int stateSequenceId = 0;


#define MAX_CLIENTS 4
std::map<IpAddress, Actor*> clients;
unsigned int numClients = 0;


Actor* actors[MAX_ACTORS]{}; // Actors created and replicated
unsigned int numActors = 0;


/*
* KEY: Simulation step each set of actors are to begin being simulated
* VALUE: Set of actors to be simulated upon reaching the correct simulation step
*/
std::map<unsigned int, std::set<Actor*>> preSpawnedActors;



/*
* Creates actor. 
* Increments numActors
* adds the created actor to actors
*/
Actor* createActor(Vector3D& pos, Vector3D& rot);
/* Destroys actor.
*  Decrements numActors.
*  removes the created actor from actors
*/
// TODO: Implement Destroy Actor
void moveActors(float deltaTime);


// ---- SERVER ---- //
int main()
{
	bool bTesting = true; // TEMP

	UDPSocket sock;
	sock.init(true);

	std::chrono::steady_clock::time_point start = std::chrono::high_resolution_clock::now();
	std::chrono::steady_clock::time_point end;

	/*
	* 1 byte : message type char
	* sizeof(unsigned int) bytes : room for stateSequenceId
	* remaining bytes. Actor replication
	* 
	* I doubt any other kind of message will exceed the size of this type of message
	*/
	char sendBuffer[1 + sizeof(unsigned int) + (MAX_ACTORS * sizeof(Actor))]{};
	sockaddr_in clientAddr;
	clientAddr.sin_family = AF_INET;
	clientAddr.sin_port = htons(4242);
	int clientAddr_len = sizeof(clientAddr);


	float elapsedTimeSinceUpdate = 0.f;
	while (true)
	{
		// -- Time Management -- //	
		end = start;
		start = std::chrono::high_resolution_clock::now();
		deltaTime = (start - end).count() / 1000000000.f;	// nanoseconds to seconds
		elapsedTimeSinceUpdate += deltaTime;

		// -- Updating Actors -- //
		moveActors(deltaTime);

		
		// -- Checking for spawn/respawn of player characters -- //
		// BUG: There are two actors being created when we create an actor here
		auto iter = clients.begin();
		while (iter != clients.end())
		{
			// TODO: Create respawn timer
			if (iter->second == nullptr)
			{
				// TODO: Properly choose a position for each client actor
				iter->second = Actor::createActorFromBlueprint('\x0', Vector3D(0.f), Vector3D(0.f), 0, false);
				actors[numActors] = iter->second;
				numActors++;
				//iter->second = createActor(position, rotation);
				//iter->second->InitializeModel("C:/Users/User/source/repos/Multiplayer-Asteroids/CommonClasses/FBX/Gear/Gear1.fbx");
				printf("Creating client actor\n");
				// QUESTION: Why would creating an actor in this way result in a nullptr error when trying to replicate
				//iter->second = new Actor(position, rotation);
				//numActors++;
				//actors[numActors] = iter->second;

				// -- Sending actor ID to client -- //
				sendBuffer[0] = 'i';
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
		char* recvBuffer = sock.recvData(numBytesRead, clientAddr);
		if (numBytesRead > 0) // Received Anything?
		{
			switch (recvBuffer[0]) // Instruction Received
			{
				case 'c': // Connect. Client wants to be added to list of connected clients
				{
					std::cout << "client connected\n"; // TODO: print client IP address
					// - Storing client ip address - //
					IpAddress ip;
					ip._in_addr = clientAddr.sin_addr;
					clients.insert({ ip, nullptr });
					numClients++;

					// - replying to message - //
					/*
					* data is message type & state of simulation so client can try to get ahead
					*/
					sendBuffer[0] = 'c';
					memcpy(sendBuffer + 1, &stateSequenceId, sizeof(unsigned int));
					sock.sendData(sendBuffer, 1 + sizeof(unsigned int), clientAddr);
					break;
				}
				case 'r': // Client sending actor movement update
				{
					// Receives client inputs and uses them to update the state of that client's playerActor
					// TODO: Handle rotation and action inputs like quitting or shooting
					IpAddress ipAddr; // That I must construct one of these feels code smelly
					ipAddr._in_addr = clientAddr.sin_addr;
					if (clients.count(ipAddr) == 0)
					{
						std::cout << "Server::main/receiving_data -- received replication of input from unknown client. Ignoring";
						break;
					}
					if (clients[ipAddr] == nullptr)
					{
						// The client attempting to replicate has no actor to move
						break; 
					}
					ActorNetData netData;
					memcpy(&netData, recvBuffer + 1, sizeof(netData));
					Actor* actor = clients[ipAddr];
					actor->setMoveDirection(netData.moveDirection);
					break;
				}
				case 's': // Client spawned an actor.
				{
					std::cout << "Server / Receiving Data / s ...\n";

					// -- Deformatting data & Spawning Actor -- // 
					NetworkSpawnData data;
					memcpy(&data, recvBuffer, sizeof(NetworkSpawnData));
					

					// TODO: temp solution to finding projetile position. May be permenant solution, but must first consider other options
					IpAddress clientIp;
					clientIp._in_addr = clientAddr.sin_addr;
					Actor* clientActor = clients[clientIp];

					Actor* projectile = Actor::createActorFromBlueprint(
						ABP_PROJECTILE, 
						clientActor->getPosition() + clientActor->getRotation() * 100.f,
						Vector3D(1.f, 0.f, 0.f), 
						0,
						false
					);
					actors[numActors] = projectile;
					numActors++;
					data.networkedActorID = projectile->getId();
					// preSpawnedActors[data.simulationStep].insert(projectile); // Creates new element if it doesn't already exist

					// -- Sending Reply to Client -- //
					char replyBuffer[sizeof(NetworkSpawnData)];
					sock.sendData(replyBuffer, sizeof(NetworkSpawnData), clientAddr);

					break;
				}
				case 't': // Case used for testing. Should not be in the final build
				{
					if (bTesting)
					{
						bTesting = false;
						std::cout << "receiving test\n";
						sendBuffer[0] = 't';
						sock.sendData(sendBuffer, 1, clientAddr);
					}
					break;
				}
			}
		}


		// -- Fixed Frequency Update -- //
		if (elapsedTimeSinceUpdate >= updatePeriod) // ~20 times a second
		{

			elapsedTimeSinceUpdate -= updatePeriod;
			stateSequenceId++;

		
			// -- Sending Snapshot to Clients -- //
			if (numClients <= 0) continue;
			// - Packing actor data - //
			sendBuffer[0] = 'r'; // 'r' --> actor replication message
			char* tempBuffer = sendBuffer;
			tempBuffer += 1; // skipping message type byte
			memcpy(tempBuffer, &stateSequenceId, sizeof(unsigned int));
			tempBuffer += sizeof(unsigned int); // Skipping state sequence ID bytes
			for (unsigned int i = 0; i < numActors; i++)
			{
				// std::cout << actors[i]->getPosition().toString() << std::endl;
				// memcpy(tempBuffer, actors[i], sizeof(Actor));
				Actor::serialize(tempBuffer, actors[i]);
				tempBuffer += sizeof(Actor);
			}
			// - Sending Actor data - //
			unsigned int bufferLen = 1 + sizeof(unsigned int) + (numActors * sizeof(Actor));
			for (auto it = clients.begin(); it != clients.end(); ++it)
			{
				//printf("Sending Snapshot\n");
				clientAddr.sin_addr = it->first._in_addr;
				sock.sendData(sendBuffer, bufferLen, clientAddr);
			}
		}//~ Fixed Update

	};//~ Main Loop

	WSACleanup();

	// Prevents window from closing too quickly. Good so I can see print statements
	char temp[200];
	std::cin >> temp;

	return 0;
};



Actor* createActor(Vector3D& pos, Vector3D& rot)
{
	actors[numActors] = new Actor(pos, rot);
	numActors++;
	return actors[numActors-1];
}


void moveActors(float deltaTime)
{
	for (unsigned int i = 0; i < numActors; i++)
	{
		Actor* actor = actors[i];
		actor->addToPosition(actor->getMoveDirection() * actor->getMoveSpeed() * deltaTime);

		if (actor->getId() != 0)
			std::cout << actor->toString() << std::endl;



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