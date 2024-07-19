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
struct ipAddress
{
public:
	in_addr _in_addr;

	bool operator<(const ipAddress& other) const
	{
		return _in_addr.S_un.S_addr < other._in_addr.S_un.S_addr;
	}
};

float updatePeriod = 1.f / 4.f; // Verbose to allow easy editing. Should be properly declared later // 20.f; 
float deltaTime = 0.f;

unsigned int stateSequenceId = 0;


#define MAX_CLIENTS 4
std::map<ipAddress, Actor*> clients;
unsigned int numClients = 0;


Actor* actors[MAX_ACTORS]{}; // Actors created and replicated
unsigned int numActors = 0;


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



		// BUG: There are two actors being created when we create an actor here
		// -- Checking for spawn/respawn of player characters -- //
		auto iter = clients.begin();
		while (iter != clients.end())
		{
			// TODO: Create respawn timer
			if (iter->second == nullptr)
			{
				// TODO: Properly choose a position for each client actor
				Vector3D position(0.f);
				Vector3D rotation(0.f);
				iter->second = createActor(position, rotation);
				printf("Creating Actor\n");
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


		// -- Receiving Data -- //
		int numBytesRead;
		char* recvBuffer = sock.recvData(numBytesRead, clientAddr);
		if (numBytesRead > 0) // Received Anything?
		{
			switch (recvBuffer[0]) // Instruction Received
			{
				// connect. Client wants to be added to list of connected clients
				case 'c':
				{
					std::cout << "client connected\n"; // TODO: print client IP address
					// - Storing client ip address - //
					ipAddress ip;
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
				case 'r':
				{
					//std::cout << "Server::main -- Received client replication" << std::endl;
					// Receives client inputs and uses them to update the state of that client's playerActor
					// TODO: Handle rotation and action inputs like quitting or shooting
					// That I must construct one of these feels code smelly
					ipAddress ipAddr;
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
					Vector3D movementDir;
					memcpy(&movementDir, recvBuffer + 1, sizeof(Vector3D));
					// std::cout << (movementDir * 70.f * deltaTime).toString() << std::endl;
					Actor* actor = clients[ipAddr];
					actor->setPosition(actor->getPosition() + (70.f * movementDir * deltaTime));
					std::cout << actor->getId() << "/" << actor->getPosition().toString() << std::endl;
					break;
				}
				case 't': // Case used for testing. Should not be in the finals build
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



		// -- Sending Data: Actor Replication -- //
		if (elapsedTimeSinceUpdate >= updatePeriod) // ~20 times a second       
		{
			elapsedTimeSinceUpdate -= updatePeriod;
			stateSequenceId++;
			std::cout << "Fixed Update: " << stateSequenceId << std::endl;

			if (numClients <= 0) continue;
	
			// - Packing actor data - //
			sendBuffer[0] = 'r'; // 'r' --> actor replication message
			char* tempBuffer = sendBuffer;
			tempBuffer += 2;
			// memcpy(sendBuffer + 1, (void*)stateSequenceId++, sizeof(unsigned int));
			for (unsigned int i = 0; i < numActors; i++)
			{
				memcpy(tempBuffer, actors[i], sizeof(Actor));
				tempBuffer += sizeof(Actor);
			}
			// - Sending Actor data - //
			unsigned int bufferLen = 1 + (numActors * sizeof(Actor));
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


