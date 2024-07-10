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



#define MAX_CLIENTS 4
std::map<ipAddress*, Actor*> clients;
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
	UDPSocket sock;
	sock.init(true);


	// -- Testing -- //
	Vector3D testPos(-.5f, 0.f, 0.f);
	Vector3D testRot(0.f);
	createActor(testPos, testRot);

	//std::chrono::steady_clock::time_point start = std::chrono::high_resolution_clock::now();
	//std::chrono::steady_clock::time_point end;
	//float seconds = 0.f;
	//----

	char sendBuffer[1 + (MAX_ACTORS * sizeof(Actor))]{};
	sockaddr_in clientAddr;
	clientAddr.sin_family = AF_INET;
	clientAddr.sin_port = htons(4242);
	int clientAddr_len = sizeof(clientAddr);


	while (true)
	{
		// -- Testing -- // QUESTION: What is the relationship between deltaTime and glfwGetTime() 
		//end = std::chrono::high_resolution_clock::now();
		//seconds = (end - start).count() / 1000000000.f;	// nanoseconds to seconds	
		////std::cout << seconds << std::endl;
		//int sign = 1; 
		//printf("numActors: %d\n", numActors);
		//for (unsigned int i = 0; i < numActors; i++)
		//{
		//	Vector3D tempPos((float)sin(seconds), (float)cos(seconds), 0.f);
		//	actors[i]->setPosition(tempPos * sign * 10.f);
		//	sign *= -1;
		//	//std::cout << tempPos.toString() << std::endl;
		//}
		//---- Testing -----//


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

				clientAddr.sin_addr = iter->first->_in_addr;
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
					// Storing client ip address
					ipAddress* ip = new ipAddress;
					ip->_in_addr = clientAddr.sin_addr;
					clients.insert({ ip, nullptr });
					numClients++;
					// replying to message

					sendBuffer[0] = 'c';
					sock.sendData(sendBuffer, 1, clientAddr);
					break;
				}
				case 'r':
				{
					// Receives client inputs and uses them to update the state of that client's playerActor
					// TODO: Handle rotation and action inputs like quitting or shooting
					
					// That I must construct one of these feels code smelly
					ipAddress ipAddr;
					ipAddr._in_addr = clientAddr.sin_addr;
					if (clients.count(&ipAddr) == 0)
					{
						std::cout << "Server::main/receiving_data -- received replication of input from unknown client. Ignoring";
						break;
					}
					Vector3D movementDir;
					memcpy(&movementDir, recvBuffer + 1, sizeof(Vector3D));


					break;
				}
			}
		}

		// -- Actor replication -- //
		// - Packing actor data
		sendBuffer[0] = 'r'; // 'r' --> actor replication message
		char* tempBuffer = sendBuffer;
		tempBuffer++;
		for (unsigned int i = 0; i < numActors; i++)
		{
			memcpy(tempBuffer, actors[i], sizeof(Actor));
			tempBuffer += sizeof(Actor);
		}
		unsigned int bufferLen = 1 + (numActors * sizeof(Actor));

		// - Replicating
		for (auto it = clients.begin(); it != clients.end(); ++it)
		{	
			clientAddr.sin_addr = it->first->_in_addr;
			sock.sendData(sendBuffer, bufferLen, clientAddr);
		}	
	};

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


