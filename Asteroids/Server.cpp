
#include <iostream>
#include <WinSock2.h>
#include <WS2tcpip.h> // inetPtons
#include <tchar.h> // _T
#include <thread>

#include "../CommonClasses/Vector.h"

#pragma comment(lib, "ws2_32.lib")


// ---- SERVER ---- //
int main()
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
		return 0;
	}
	else
	{
		std::cout << "Winsock DLL found" << std::endl;
		std::cout << "Status: " << wsaData.szSystemStatus << std::endl;
	}

	
	// -- 2. Create the socket -- //
	SOCKET serverSocket = INVALID_SOCKET;
	serverSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (serverSocket == INVALID_SOCKET)
	{
		std::cout << "Error at socket(): " << WSAGetLastError() << std::endl;
		WSACleanup();
		return 0;
	}
	else
	{
		std::cout << "socket() OK" << std::endl;
	}

	
	// -- 3. Binding Server to Address -- //
	sockaddr_in socketInitializer;
	socketInitializer.sin_family = AF_INET;
	InetPtonW(AF_INET, _T("192.168.2.74"), &socketInitializer.sin_addr.s_addr);
	socketInitializer.sin_port = htons(6969);
	if (bind(serverSocket, (SOCKADDR*)&socketInitializer, sizeof(socketInitializer)) == SOCKET_ERROR)
	{
		std::cout << "bind() failed: " << WSAGetLastError() << std::endl;
		closesocket(serverSocket);
		WSACleanup();
		return 0;
	}
	else
	{
		std::cout << "bind() OK" << std::endl;
	}


	// -- Sending and Receiving Data -- //
	char receiveBuffer[200]{};
	sockaddr_in clientAddr;
	int clientAddr_len = sizeof(clientAddr);
	while (true)
	{
		int bytesReceived = recvfrom(serverSocket, receiveBuffer, 200, 0, (SOCKADDR*)&clientAddr, &clientAddr_len);
		if (bytesReceived < 0)
		{
			std::cout << "Unable to receive datagram" << std::endl;
			WSACleanup();
			return 0;
		}

		printf("%s\n", receiveBuffer);

	};


	WSACleanup();
	return 0;
};