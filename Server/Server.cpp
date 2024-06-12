
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
	
    struct addrinfo hints, * res, * p;
    int status;
    char ipstr[INET6_ADDRSTRLEN];

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; // AF_INET or AF_INET6 to force version
    hints.ai_socktype = SOCK_STREAM;

    if ((status = getaddrinfo("", NULL, &hints, &res)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        return 2;
    }

    // printf("IP addresses for %s:\n\n", "");

    for (p = res; p != NULL; p = p->ai_next) {
        void* addr;
        char* ipver;

        // get the pointer to the address itself,
        // different fields in IPv4 and IPv6:
        if (p->ai_family == AF_INET) { // IPv4
            struct sockaddr_in* ipv4 = (struct sockaddr_in*)p->ai_addr;
            addr = &(ipv4->sin_addr);
            // ipver = "IPv4";
        }
        else { // IPv6
            struct sockaddr_in6* ipv6 = (struct sockaddr_in6*)p->ai_addr;
            addr = &(ipv6->sin6_addr);
            // ipver = "IPv6";
        }

        // convert the IP to a string and print it:
        inet_ntop(p->ai_family, addr, ipstr, sizeof ipstr);
        printf("  %s: %s\n", ipver, ipstr);
    }

    freeaddrinfo(res); // free the linked list

    return 0;








    UDPSocket sock;


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