
#include <iostream>
#include <WinSock2.h>
#include <WS2tcpip.h> // inetPtons
#include <tchar.h> // _T
#include <thread>

#include "../CommonClasses/Actor.h"
#include "../CommonClasses/Vector.h"
#include "../CommonClasses/UDPSocket.h"


// Linking (What does this have to do with linking????
#pragma comment(lib, "ws2_32.lib")


#define MAX_CLIENTS 4


/*
* Stores the client IP 
* instantiates a client actor.
* Sends a message with the newly created actor's ID so the client knows
* which Actor it's controlling
*/
void handleNewClient(unsigned int newClientIPAddress);


// ---- SERVER ---- //
int main()
{
	

	UDPSocket sock;
	sock.init();


	unsigned long clientIPAddresses[MAX_CLIENTS]{};
	unsigned int numClients = 0;




	// -- Sending and Receiving Data -- //
	char receiveBuffer[200]{};
	sockaddr_in clientAddr;
	int clientAddr_len = sizeof(clientAddr);
	while (true)
	{
		const char* buffer;
		int numBytesRead;
		buffer = sock.recvData(numBytesRead, clientAddr);
		printf("%i\n", numBytesRead);
		/*if (numBytesRead > 0)
		{
			printf("%s\n", buffer);
		}*/
	};



	WSACleanup();
	return 0;
};


void handleNewClient(unsigned int newClientIPAddress)
{

}