#include "UDPSocket.h"
#include <pch.h>
#include <iostream>
#include <WS2tcpip.h> // inetPtons
#include <tchar.h>

#pragma comment(lib, "ws2_32.lib")


//  UDPSocket::
int UDPSocket::init()
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
		return error;
	}
	else
	{
		std::cout << "socket() OK" << std::endl;
	}

	// -- 3. Binding Socket to Address -- //
	// - 3.1 Retreiving device LAN IP address - //
	addrinfo hints, *res, *p;
	int status;
	char ipstr[INET6_ADDRSTRLEN];

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; // pNodeName will be set to `localIPAddress`, we'll receive our computer's IP address on our LAN
	
	/* One or both of the first params MUST be NULL.
	* Otherwise, there isn't any point in calling a function 
	* to list options
	*/
	if ((status = getaddrinfo("", NULL, &hints, &res)) != 0)
	{
		// printf("%s", gai_strerror(status));
		return -1;
	}
	
	for (p = res; p != NULL; p = p->ai_next)
	{
		void* addr;
		const char* ipver;

		// get the pointer to the address itself
		if (p->ai_family == AF_INET) // IPv4
		{
			struct sockaddr_in* ipv4 = (struct sockaddr_in*)p->ai_addr;
			addr = &(ipv4->sin_addr);
			ipver = "IPv4";
		}
		else
		{
			struct sockaddr_in6* ipv6 = (struct sockaddr_in6*)p->ai_addr;
			addr = &(ipv6->sin6_addr);
			ipver = "IPv6";
		}

		inet_ntop(p->ai_family, addr, ipstr, sizeof(ipstr));
		printf("  %s:  %s\n", ipver, ipstr);

	}


	freeaddrinfo(res);



	return 0;

	sockaddr_in socketInitializer;
	socketInitializer.sin_family = AF_INET;
	InetPtonW(AF_INET, _T("192.168.2.74"), &socketInitializer.sin_addr.s_addr);
	socketInitializer.sin_port = htons(6969);
	if (bind(sock, (SOCKADDR*)&socketInitializer, sizeof(socketInitializer)) == SOCKET_ERROR)
	{
		std::cout << "bind() failed: " << WSAGetLastError() << std::endl;
		closesocket(sock);
		WSACleanup();
		return 0;
	}
	else
	{
		std::cout << "bind() OK" << std::endl;
	}
};


/*

*/