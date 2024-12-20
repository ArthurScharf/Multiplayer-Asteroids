#pragma once

#include <WinSock2.h>

#pragma comment(lib, "ws2_32.lib")


class UDPSocket
{
private:
	SOCKET sock;
	const char* IPv4;
	/* Number of bytes allocated to the recvBuffer in UDPSocket::recvData.
	*  Maximum Packet size (of fragmentation is allowed) is 65,535 (2^16).
	*/
	int bufferSize = 256;

public:
	// returns 0 if ok. anything means error
	int init(bool bIsServer);

	void sendData(const char* buffer, unsigned int bufferLen, sockaddr_in& recvAddr);
	// returns bytes read as dynamic array
	char* recvData(int& numBytesRead, sockaddr_in& sendingSockAddr);
	
public:
	inline const char* getIPv4() { return IPv4; }

	inline void setRecvBufferSize(int numBytes) { bufferSize = numBytes; }
};