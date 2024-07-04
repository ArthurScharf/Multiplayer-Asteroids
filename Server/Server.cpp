
#include <iostream>
#include <map>
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


#define MAX_CLIENTS 4


Actor* actors[MAX_ACTORS]{}; // Actors created and replicated
unsigned int numActors = 0;


/*
* Exists so client IP's can be used as key's in a client to actor map.
*/
struct ipAddress
{
	in_addr _in_addr;

	bool operator<(const ipAddress& other) const
	{
		return _in_addr.S_un.S_addr < other._in_addr.S_un.S_addr;
	}
};
/*
* Relation between a clients IP address and the actor they're controlling.
* Stored in network byte order. No reason to spend time changing it's format
*/
std::map<ipAddress, Actor*> clients;
unsigned int numClients = 0;


/*
* Creates actor. Guarantees that numActors will be incremented 
*/
Actor* createActor(Vector3D& pos, Vector3D& rot);





// ---- SERVER ---- //
int main()
{
	UDPSocket sock;
	sock.init(true);


	// -- Sending and Receiving Data -- //
	char recvBuffer[1 + (MAX_ACTORS * sizeof(Actor))]{};
	char sendBuffer[1 + (MAX_ACTORS * sizeof(Actor))]{};
	char* tempBuffer; // Used to populate send buffer without incrementing send buffer
	sockaddr_in clientAddr;
	int clientAddr_len = sizeof(clientAddr);
	while (true)
	{
		tempBuffer = sendBuffer + 1;

		// -- Receiving Data -- //
		int numBytesRead = 0;
		memcpy(recvBuffer, sock.recvData(numBytesRead, clientAddr), numBytesRead); // Will numBytesRead be set before memcpy be called? It must bc we cant call the outer function until the inner is resolved
		char* parsedBuffer = (char*)malloc(1);
		if (numBytesRead != 0) // Received Anything?
		{
			if (recvBuffer[0] == '0') // Command received
			{
				ipAddress ip;
				ip._in_addr = clientAddr.sin_addr;
				// New client connection
				if (clients.count(ip) != 0) // Error. Client that is already connected wants to connect
				{
					continue;
					// TODO: reply with an error message that tells the client they're already connected
					// TODO: Remove the client from the client pool?
				}
				else // new client
				{
					Vector3D _position(0.f);
					Vector3D _rotation(0.f);
					// Creating new clients actor
					Actor* clientActor = new Actor(_position, _rotation); // TODO: Properly choose the spawn location for the player's actor
					clients.insert(std::make_pair(ip, clientActor)); // Creating the associating
					actors[numActors] = clientActor;

					// Replying with the clients controlled actor
					sendBuffer[0] = '0'; // Sending command
					sendBuffer[1] = '0'; // Command: Connection reply
					memcpy(sendBuffer + 2, clientActor, sizeof(Actor));
					sock.sendData(sendBuffer, 2 + sizeof(Actor), clientAddr);

					std::cout << "Server::main -- client connected\n";
				}
			}
			if (recvBuffer[0] == '1') // Actor replication received
			{
				// TODO

			}
		}

		
		continue; // TESTING CONNECTION RN


		// -- Actor replication -- //
		// Packing actor data
		sendBuffer[0] = '1'; // Actor replication
		char* tempBuffer = sendBuffer;
		tempBuffer++;
		for (unsigned int i = 0; i < numActors; i++)
		{
			memcpy(tempBuffer, actors[i], sizeof(Actor));
			tempBuffer += sizeof(Actor);
		}
		unsigned int bufferLen = 1 + (numActors * sizeof(Actor));

		// Replicating
		sockaddr_in clientSock{};
		clientSock.sin_family = AF_INET;
		clientSock.sin_port = htons(4242);
		for (auto it = clients.begin(); it != clients.end(); ++it)
		{	
			clientSock.sin_addr = it->first._in_addr;
			sock.sendData(sendBuffer, bufferLen, clientSock);
		}	
	};

	WSACleanup();
	return 0;
};



Actor* createActor(Vector3D& pos, Vector3D& rot)
{

	return nullptr;
}
