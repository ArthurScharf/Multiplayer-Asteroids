#pragma once

#include <WinSock2.h>

#pragma comment(lib, "ws2_32.lib")

class UDPSocket
{
private:
	SOCKET sock;

public:
	int init();



};