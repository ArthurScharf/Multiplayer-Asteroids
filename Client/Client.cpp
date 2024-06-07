#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <WinSock2.h>
#include <WS2tcpip.h> // inetPtons
#include <tchar.h> // _T
#include <thread>

#include "../CommonClasses/Actor.h"
#include "../CommonClasses/Vector.h"
#include "../CommonClasses/Serialization/Serializer.h"

#pragma comment(lib, "ws2_32.lib") // What is this doing?

const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

bool bWinsockLoaded = false;

Vector3D position; // Player position. A temporary place for this. Will probably go into an Actor class later on
const float playerSpeed = 25.f; // The rate at which the player changes position



// -- Forward Declaring Functions -- //
void processInput(GLFWwindow* window);
void terminate();



/* It is the job of the client's main code to organize and denote the structure
*  Of the serialized data being sent. In other words, it must
*  organize the serialized actors and instructions with the same custom protocol
*  that the server does
*/
// ---- CLIENT ---- //
int main()
{	
	Actor* actor = new Actor(Vector3D(1.f, 1.f, 1.f), Vector3D(0.f), 314);
	std::cout << "ID:" << actor->getId() << " / Pos:" << actor->getPosition().toString() << " / Rot: " << actor->getRotation().toString() << std::endl;

	char* buffer = new char[sizeof(Actor)];
	unsigned int bytesStored = Actor::serialize(buffer, actor);
	// std::cout << "bytes stored: " << bytesStored << std::endl;
	
	delete actor;
	actor = Actor::deserialize(buffer, bytesStored);
	//std::cout << "bytes read: " << bytesStored << std::endl;
	std::cout << "ID: " << actor->getId() << "/Pos: " << actor->getPosition().toString() << "/ Rot: " << actor->getRotation().toString() << std::endl;

	

	return 0;
	/**/

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
		terminate();
		return 0;
	}

	

	// ---------- Networking ---------- //
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
		bWinsockLoaded = true;
	}


	// -- 2. Create the socket -- //
	SOCKET clientSocket = INVALID_SOCKET;
	clientSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (clientSocket == INVALID_SOCKET)
	{
		std::cout << "Error at socket(): " << WSAGetLastError() << std::endl;
		terminate();
		return 0;
	}
	else
	{
		std::cout << "socket() OK" << std::endl;
	}


	// ToDo: Start seperate thread to receive data from server.


	// -- 3. Sending Messages -- //
	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	InetPtonW(AF_INET, _T("192.168.2.74"), &serverAddr.sin_addr.s_addr);
	serverAddr.sin_port = htons(6969);
	while (!glfwWindowShouldClose(window))
	{
		processInput(window);

		std::cout << position.toString() << std::endl;


		// ToDo: Construct client status buffer
		// Update: Remember to set first byte in buffer with buffer type
		char buffer[200]{};
		// sprintf_s(buffer, "%6.5f %6.5f %6.5f", position.x, position.y, position.z);

		int bytesSent = sendto(clientSocket, (const char*)buffer, strlen(buffer), 0, (SOCKADDR*)&serverAddr, sizeof(serverAddr));
		if (!bytesSent)
		{
			std::cout << "Error transmitting data" << std::endl;
			terminate();
			return 0;
		}
		
		glfwPollEvents();
		glfwSwapBuffers(window);
	}

	terminate();
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


void terminate()
{
	if (bWinsockLoaded) WSACleanup();
	glfwTerminate();
}