
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
void destroyActor(Actor* actor);




// ---- SERVER ---- //
int main()
{
	UDPSocket sock;
	sock.init(true);


	char sendBuffer[1 + (MAX_ACTORS * sizeof(Actor))]{};
	sockaddr_in clientAddr;
	clientAddr.sin_family = AF_INET;
	clientAddr.sin_port = htons(4242);

	int clientAddr_len = sizeof(clientAddr);
	while (true)
	{

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
				iter++;
			}
		}



		// -- Receiving Data -- //
		int numBytesRead;
		char* recvBuffer = sock.recvData(numBytesRead, clientAddr);
		if (numBytesRead > 0) // Received Anything?
		{
			printf("Message: %c\n", recvBuffer[0]);
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
				// replying to message

				sendBuffer[0] = 'c';
				sock.sendData(sendBuffer, 1, clientAddr);
				break;
			}
			case 'r':
			{
				// Receives client inputs and uses them to update the state of that client's playerActor

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
	return 0;
};



Actor* createActor(Vector3D& pos, Vector3D& rot)
{
	actors[numActors] = new Actor(pos, rot);
	numActors++;
	return actors[numActors-1];
}

void destrorActor(Actor* actor)
{
	// TODO
}
