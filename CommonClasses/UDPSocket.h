#pragma once

#include <WinSock2.h>

#pragma comment(lib, "ws2_32.lib")

// DEPRECATED. use bufferSize
#define BUFFER_SIZE 200

class UDPSocket
{
private:
	SOCKET sock;
	const char* IPv4;
	/* Number of bytes allocated to the recvBuffer in UDPSocket::recvData.
	*  Default value is 256 (chosen arbitrarily).
	*  Maximum Packet size (of fragmentation is allowed) is 65,535 (2^16).
	*/
	static const int bufferSize = 2000; // TEST value. Arbitrary

public:
	// returns 0 if ok. anything means error
	int init(bool bIsServer);

	void sendData(const char* buffer, unsigned int bufferLen, sockaddr_in& recvAddr);
	// returns bytes read as dynamic array
	char* recvData(int& numBytesRead, sockaddr_in& sendingSockAddr);
	
public:
	inline const char* getIPv4() { return IPv4; }
};