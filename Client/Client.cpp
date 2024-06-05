
#include <iostream>
#include <WinSock2.h>
#include <WS2tcpip.h> // inetPtons
#include <tchar.h> // _T
#include <thread>

#include "../CommonClasses/Vector.h"

#pragma comment(lib, "ws2_32.lib")


// ---- CLIENT ---- //
int main()
{

	// -- 1. Load the DL -- //
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
	SOCKET clientSocket = INVALID_SOCKET;
	clientSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (clientSocket == INVALID_SOCKET)
	{
		std::cout << "Error at socket(): " << WSAGetLastError() << std::endl;
		WSACleanup();
		return 0;
	}
	else
	{
		std::cout << "socket() OK" << std::endl;
	}


	Vector3D position(0.f, 0.f, 0.f);
	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	InetPtonW(AF_INET, _T("192.168.2.74"), &serverAddr.sin_addr.s_addr);
	serverAddr.sin_port = htons(6969);
	while (true)
	{
		char buffer[200]{};
		sprintf_s(buffer, "%6.1f %6.1f %6.1f", position.x, position.y, position.z);
		int bytesSent = sendto(clientSocket, (const char*)buffer, strlen(buffer), 0, (SOCKADDR*)&serverAddr, sizeof(serverAddr));
		if (!bytesSent)
		{
			std::cout << "Error transmitting data" << std::endl;
			WSACleanup();
			return 0;
		}
		
		position += 1.f;
	}

	WSACleanup();
	return 0;
};