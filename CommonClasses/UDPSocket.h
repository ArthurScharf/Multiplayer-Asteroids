#pragma once

#include <WinSock2.h>

#pragma comment(lib, "ws2_32.lib")

#define BUFFER_SIZE 200

class UDPSocket
{
private:
	SOCKET sock;

public:
	int init();

	void sendData(const char* buffer, unsigned int bufferLen);

	/*
	* returns bytes read.
	* 
	*/
	const char* recvData(int& numBytesRead, sockaddr_in& sendingSockAddr);
};