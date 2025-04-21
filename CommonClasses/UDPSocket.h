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

	void sendData(const char* buffer, unsigned int bufferLen, sockaddr_in& recvAddr);
	
	/*
	* @brief reads 'numBytesReadable' bytes into 'buffer'.
	*/
	void recvData(char* buffer, int numBytesReadable, int& numBytesRead, sockaddr_in& sendingSockAddr);
	
public:
	inline const char* getIPv4() { return IPv4; };
};