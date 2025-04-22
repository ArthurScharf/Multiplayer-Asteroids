#pragma once

#include <WinSock2.h>

#pragma comment(lib, "ws2_32.lib")


class UDPSocket
{
private:
	SOCKET sock;
	const char* IPv4;

public:
	// returns 0 if ok. anything means error
	int init(bool bIsServer);

	// returns number of bytes sent
	int sendData(const char* buffer, unsigned int bufferLen, sockaddr_in& recvAddr);
	
	/*
	* @brief reads 'numBytesReadable' bytes into 'buffer'.
	* 
	* @param numBytesReadable size of the buffer in bytes
	*/
	int recvData(char* buffer, int numBytesReadable, sockaddr_in& sendingSockAddr);

public:
	inline const char* getIPv4() { return IPv4; };
};