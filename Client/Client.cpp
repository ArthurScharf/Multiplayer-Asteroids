#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <WinSock2.h>
#include <WS2tcpip.h> // inetPtons
#include <tchar.h> // _T
#include <thread>


#include "../CommonClasses/Vector.h"

#pragma comment(lib, "ws2_32.lib")


const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

bool bWinsockLoaded = false;

//Vector3D position(5.f, 3.f, 11.f);
Vector3D position;
const float playerSpeed = 25.f; // The rate at which the player changes position


void Terminate();
void processInput(GLFWwindow* window);


// ---- CLIENT ---- //
int main()
{

	// ---------- OpenGL ---------- //
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// Creating the window 
	GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Asteroids", NULL, NULL);
	if (window == NULL)
	{
		std::cout << "main(): Failed to create window" << std::endl;
		glfwTerminate();
		return 0;
	}
	glfwMakeContextCurrent(window); // Why are we doing this again?
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);


	// Checking to see if GLAD loaded
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		Terminate();
		return 0;
	}




	// ---------- Networking ---------- //
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
		bWinsockLoaded = true;
	}


	// -- 2. Create the socket -- //
	SOCKET clientSocket = INVALID_SOCKET;
	clientSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (clientSocket == INVALID_SOCKET)
	{
		std::cout << "Error at socket(): " << WSAGetLastError() << std::endl;
		Terminate();
		return 0;
	}
	else
	{
		std::cout << "socket() OK" << std::endl;
	}

	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	InetPtonW(AF_INET, _T("192.168.2.74"), &serverAddr.sin_addr.s_addr);
	serverAddr.sin_port = htons(6969);
	while (!glfwWindowShouldClose(window))
	{
		processInput(window);

		std::cout << position.toString() << std::endl;

		char buffer[200]{};
		sprintf_s(buffer, "%6.1f %6.1f %6.1f", position.x, position.y, position.z);
		int bytesSent = sendto(clientSocket, (const char*)buffer, strlen(buffer), 0, (SOCKADDR*)&serverAddr, sizeof(serverAddr));
		if (!bytesSent)
		{
			std::cout << "Error transmitting data" << std::endl;
			Terminate();
			return 0;
		}
		
		glfwPollEvents();
		glfwSwapBuffers(window);
	}

	Terminate();
	return 0;
};


void processInput(GLFWwindow* window)
{
	Vector3D movementDir;

	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		movementDir += Vector3D(0.f, 1.f, 0.f);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		movementDir += Vector3D(0.f, -1.f, 0.f);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		movementDir += Vector3D(1.f, 0.f, 0.f);
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		movementDir += Vector3D(-1.f, 0.f, 0.f);

	movementDir.Normalize();
	position = position + (movementDir * playerSpeed);
}


void Terminate()
{
	if (bWinsockLoaded) WSACleanup();
	glfwTerminate();
}