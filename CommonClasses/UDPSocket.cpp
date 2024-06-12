#include "UDPSocket.h"
#include <pch.h>
#include <iostream>
#include <WS2tcpip.h> // inetPtons
#include <tchar.h>

#pragma comment(lib, "ws2_32.lib")



int UDPSocket::init(bool bIsServer)
{
	// -- 1. Load the DLL -- //
	int error = 0;
	WORD wVersionRequested = MAKEWORD(2, 2);
	WSADATA wsaData;
	error = WSAStartup(
		wVersionRequested,
		&wsaData
	);
	if (error != 0)
	{
		std::cout << "Winsock DLL not found" << std::endl;
		return error;
	}
	else
	{
		std::cout << "Winsock DLL found" << std::endl;
		std::cout << "Status: " << wsaData.szSystemStatus << std::endl;
	}

	// -- 2. Create the socket -- //
	sock = INVALID_SOCKET;
	sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sock == INVALID_SOCKET)
	{
		std::cout << "Error at socket(): " << WSAGetLastError() << std::endl;
		WSACleanup();
		return 1;
	}
	else
	{
		std::cout << "socket() OK" << std::endl;
	}

	// -- 3. Binding Socket to Address -- //
	// Retreiving device LAN IP address
	addrinfo hints, * res;
	int status;
	char ipstr[INET_ADDRSTRLEN];

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE; // pNodeName will be set to `localIPAddress`, we'll receive our computer's IP address on our LAN
	
	/* One or both of the first params MUST be NULL.
	* Otherwise, there isn't any point in calling a function 
	* to list options
	*/
	const char* port = bIsServer ? "6969" : "4242";
	if ((status = getaddrinfo("", port, &hints, &res)) != 0)
	{
		std::cout << gai_strerror(status) << std::endl;
		return status;
	}


	char ipStr[INET_ADDRSTRLEN];
	void* addr;

	// -- Setting IPv4 -- //
	sockaddr_in* sockAddr = (sockaddr_in*)(res->ai_addr); // Since memory in both is contiguous, we can cast it and it'll populate correctly??
	addr = &sockAddr->sin_addr; // Why am I doing this?
	inet_ntop(AF_INET, sockAddr, ipStr, sizeof(ipStr));
	printf("socket IP: %s\n", ipStr);



	// Binding
	if (bind(sock, res->ai_addr, res->ai_addrlen) == SOCKET_ERROR)
	{
		std::cout << "Server::bind() failed: " << WSAGetLastError() << std::endl;
		closesocket(sock);
		WSACleanup();
		return WSAGetLastError();
	}
	else
	{
		std::cout << "Server::bind() OK" << std::endl;
	}

	freeaddrinfo(res);
	return 0;
};


void UDPSocket::test()
{
	addrinfo hints, * res, * p;
	int status;
	char ipstr[INET6_ADDRSTRLEN];

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC; // AF_INET or AF_INET6 to force version
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	if ((status = getaddrinfo("", NULL, &hints, &res)) != 0) {
		// fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
		std::cout << gai_strerror(status) << std::endl;
		return;
	}
	

	// printf("IP addresses for %s:\n\n", argv[1]);

	for (p = res; p != NULL; p = p->ai_next) {
		void* addr{};
		char* ipver[5];

		// get the pointer to the address itself,
		// different fields in IPv4 and IPv6:
		if (p->ai_family == AF_INET) { // IPv4
			struct sockaddr_in* ipv4 = (struct sockaddr_in*)p->ai_addr;
			addr = &(ipv4->sin_addr);
			// strcpy_s(ipver, sizeof("IPv4"), "IPv4");
		}
		else { // IPv6
			struct sockaddr_in6* ipv6 = (struct sockaddr_in6*)p->ai_addr;
			addr = &(ipv6->sin6_addr);
			// strcpy_s(ipver, sizeof("IPv6"), "IPv6");
		}

		// convert the IP to a string and print it:
		inet_ntop(p->ai_family, addr, ipstr, sizeof ipstr);
		printf("  %s: %s\n", ipver, ipstr);
	}

	freeaddrinfo(res); // free the linked list
}


void UDPSocket::sendData(const char* buffer, unsigned int bufferLen, sockaddr_in& recvAddr)
{

	// char buffer[BUFFER_SIZE];
	
	//sendto(
	//	buffer,
	//	BUFFER_SIZE,

	//)
	
}

const char* UDPSocket::recvData(int& numBytesRead, sockaddr_in& sendingSockAddr)
{
	char buffer[BUFFER_SIZE];
	int sockAddr_len = sizeof(sendingSockAddr);
	numBytesRead = recvfrom(sock, buffer, BUFFER_SIZE, 0, (SOCKADDR*)&sendingSockAddr, &sockAddr_len);
	return buffer;
}


