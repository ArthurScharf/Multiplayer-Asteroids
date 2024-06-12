#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <WinSock2.h>
#include <WS2tcpip.h> // inetPtons
#include <tchar.h> // _T
#include <thread>

#include "../CommonClasses/Actor.h"
#include "../CommonClasses/UDPSocket.h"
#include "../CommonClasses/Vector.h"
#include "../CommonClasses/Serialization/Serializer.h"

#pragma comment(lib, "ws2_32.lib") // What is this doing?

const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

bool bWinsockLoaded = false;

Vector3D position; // Player position. A temporary place for this. Will probably go into an Actor class later on
const float playerSpeed = 25.f; // The rate at which the player changes position

Actor* actors;
// The actor controlled by the client
Actor* playerActor;


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

	// ---------- Game State ---------- //
	playerActor = new Actor(
		Vector3D(0.f),
		Vector3D(0.f)
	);

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
	}

	

	// ---------- Networking ---------- //
	UDPSocket sock;
	if (sock.init(false) != 0); // Initializes the socket to the LAN IP on port 4242
	{
		std::cout << "main -- failed to init socket" << std::endl;
		terminate();
	}


	/* Properly formatting received IP address string and placing it in sockaddr_in struct */
	sockaddr_in serverAddr;
	std::cout << "Enter server IP address: ";
	wchar_t wServerAddrStr[INET_ADDRSTRLEN];
	std::wcin >> wServerAddrStr;
	PCWSTR str(wServerAddrStr);
	InetPtonW( AF_INET, str, &(serverAddr.sin_addr) );
	serverAddr.sin_port = htons(6969);

	


	// -- 4. Sending & Receiving Messages -- //
	while (!glfwWindowShouldClose(window))
	{
		processInput(window);

		/* Since recvfrom isn't blocking, we can do the recv code in here */


		// ToDo: Construct client status buffer
		// Update: Remember to set first byte in buffer with buffer type
		char buffer[sizeof(Actor)];
		Actor::serialize(buffer, playerActor);
		sock.sendData(buffer, sizeof(Actor), serverAddr);
		
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
	playerActor->setPosition(playerActor->getPosition() + (movementDir * playerSpeed));
}


void terminate()
{
	if (bWinsockLoaded) WSACleanup();
	glfwTerminate();
}