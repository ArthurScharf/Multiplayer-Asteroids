#pragma once

#include <WinSock2.h>

#pragma comment(lib, "ws2_32.lib")

#define BUFFER_SIZE 200

class UDPSocket
{
private:
	SOCKET sock;
	const char* IPv4;

public:
	// returns 0 if ok. anything means error
	int init(bool bIsServer);

	void sendData(const char* buffer, unsigned int bufferLen, sockaddr_in& recvAddr);

	/*
	* returns bytes read.
	* 
	*/
	const char* recvData(int& numBytesRead, sockaddr_in& sendingSockAddr);
	

public:
	inline const char* getIPv4() { return IPv4; }
};