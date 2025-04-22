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
	// Set non-blocking mode
	unsigned long nonBlocking = 1;
	if (ioctlsocket(sock, FIONBIO, &nonBlocking) == SOCKET_ERROR)
	{
		terminate();
		return 2;
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

	// -- Setting IPv4 -- //
	sockaddr_in* sockAddr = (sockaddr_in*)(res->ai_addr); // Since memory in both is contiguous, we can cast it and it'll populate correctly??
	inet_ntop(AF_INET, &sockAddr->sin_addr, ipStr, sizeof(ipStr));
	printf("socket IP: %s\n", ipStr);

	

	// -- Binding -- //
	if (bind(sock, res->ai_addr, res->ai_addrlen) == SOCKET_ERROR)
	{
		std::cout << "bind() failed: " << WSAGetLastError() << std::endl;
		closesocket(sock);
		WSACleanup();
		return WSAGetLastError();
	}
	else
	{
		std::cout << "bind() OK" << std::endl;
	}

	freeaddrinfo(res);
	return 0;
};



int UDPSocket::sendData(const char* buffer, unsigned int bufferLen, sockaddr_in& recvAddr)
{
	if (buffer[0] == MSG_RPC)
	{
		std::cout << "UDPSocket::sendData" << std::endl;
	}
	return sendto(sock, buffer, bufferLen, 0, (sockaddr*)&recvAddr, sizeof recvAddr);	
}




int UDPSocket::recvData(char* buffer, int numBytesReadable, sockaddr_in& sendingSockAddr)
{
	if (buffer == nullptr)
	{
		std::cout << "UDPSocket::recvData -- ERROR: passed uninitialized buffer" << std::endl;
	}

	int sockAddr_len = sizeof(sendingSockAddr);
	return recvfrom(sock, buffer, numBytesReadable, 0, (SOCKADDR*)&sendingSockAddr, &sockAddr_len);
}


