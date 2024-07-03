#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <WinSock2.h>
#include <WS2tcpip.h> // inetPtons
#include <tchar.h> // _T
#include <thread>
#include <map>

#include "Camera.h"

#include "../CommonClasses/Actor.h"
#include "../CommonClasses/Definitions.h"
#include "../CommonClasses/UDPSocket.h"
#include "../CommonClasses/Vector.h"
#include "../CommonClasses/Serialization/Serializer.h"

#pragma comment(lib, "ws2_32.lib") // What is this doing?

const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

bool bWinsockLoaded = false;

Vector3D position; // Player position. A temporary place for this. Will probably go into an Actor class later on
const float playerSpeed = 25.f; // The rate at which the player changes position

// We need these here so updateActors can access them //
//Actor* actors[MAX_PLAYERS]; // Allocates local memory for pointers to actor pointers
std::map<unsigned int, Actor*> actorMap;
Actor* playerActor = nullptr;


// -- Forward Declaring Functions -- //
void processInput(GLFWwindow* window);
void terminateProgram();
void updateActors(char* buffer, int bufferLen);
void handleCommand(char* buffer, unsigned int bufferLen);


/* It is the job of the client's main code to organize and denote the structure
*  Of the serialized data being sent. In other words, it must
*  organize the serialized actors and instructions with the same custom protocol
*  that the server does
*/
// ---- CLIENT ---- //
int main()
{
	// ---------- Game State ---------- //
	// playerActor = (Actor*)malloc(sizeof(Actor)); // The actor controlled by the client
	playerActor = nullptr; // nullptr used to know if client is connected

	// ---------- Networking ---------- //
	UDPSocket sock;
	sock.init(false);

	/* Properly formatting received IP address string and placing it in sockaddr_in struct */
	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	std::cout << "Enter server IP address: ";
	wchar_t wServerAddrStr[INET_ADDRSTRLEN];
	std::wcin >> wServerAddrStr;
	PCWSTR str(wServerAddrStr);
	InetPtonW(AF_INET, str, &(serverAddr.sin_addr));
	serverAddr.sin_port = htons(6969);

	// PROOF: We're storing the Server info correctly
	//char ip[INET6_ADDRSTRLEN];
	//inet_ntop(AF_INET, (sockaddr*)&serverAddr.sin_addr, ip, INET6_ADDRSTRLEN);
	// printf("IP: %s   Port: %d", &ip, ntohs(serverAddr.sin_port));



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
	
	// Camera
	//Camera camera(
	//	glm::vec3(0.f, 0.f, 3.f), // Camera Position
	//	glm::vec3(0.f, 1.f, 0.f), // Camera Up
	//	-90.f,
	//	0.f
	//);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	
	// Checking to see if GLAD loaded
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		terminateProgram();
	}

	// OpenGL Settings
	glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
	glEnable(GL_DEPTH_TEST);

	// Compiling GLSL Shaders 
	Shader shader("GLSL Shaders/Vertex.txt", "Shaders/Fragment.txt");

	// Loading Models. Newly constructed actors have their model pointer copied. Faster than loading each time an actor is created
	//Model model_ship();
	//Model model_asteroid();
	//Model model_missile();
	
	//Vector3D position(0.f);
	//Vector3D rotation(0.f);
	//Actor testActor(position, rotation, "test");	
	Model model_test("../CommonClasses/FBX/chair/chair.fbx");

	// Main loop
	while (!glfwWindowShouldClose(window))
	{
		processInput(window);



		// -- 4.1. Rendering Actors -- //
		glClearColor(0.1f, 0.1f, 0.1f, 1.f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glm::mat4 model = glm::mat4(1.f);
		glm::mat4 view = glm::mat4(1.f);
		glm::mat4 projection = glm::mat4(1.f);

		// Vertex Transformation
		//model = glm::translate(model, glm::vec3(0.f));
		//view = camera.GetViewMatrix();
		//projection = glm::perspective(glm::radians(camera.zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.f);




		// -- 4.2. Sending & Receiving Messages -- 
		//		Schema: First Actor in Buffer is always clients actor
		
		// -- Receiving Data from Server -- //
		int numBytesRead = 0;
		sockaddr_in recvAddr;
		char* recvBuffer{};
		recvBuffer = sock.recvData(numBytesRead, recvAddr);
		if (numBytesRead > 0) 
		{
			if (recvBuffer[0] == '0') // Command Received
				handleCommand(++recvBuffer, numBytesRead); // We increment once before passing since the we don't need the 'command received' byte to be present
			else if (recvBuffer[0] == '1') // Actor replication
				updateActors(recvBuffer, numBytesRead);
		}
		// -- Sending Data to Server -- //
		char sendBuffer[2 + (MAX_ACTORS * sizeof(Actor))]; // I doubt command data will ever exceed this size
		// No player actor --> The server hasn't given us one yet; we're not connected
		if (!playerActor)
		{
			//printf("main -- Connecting");
			sendBuffer[0] = '0'; // Connect
			sock.sendData(sendBuffer, 1, serverAddr);
		}
		else
		{
			//printf("main -- Replicating");
			Actor::serialize(sendBuffer, playerActor); // static method so knows how large buffer SHOULD be.
			sock.sendData(sendBuffer, sizeof(Actor), serverAddr);
		}

		glfwPollEvents();
		glfwSwapBuffers(window);
	}

	terminateProgram();

	// Prevents window from closing too quickly. Good so I can see print statements
	char temp[200];
	std::cin >> temp;

	return 0;
};


void processInput(GLFWwindow* window)
{
	std::cout << "  processInput\n";
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);
	if (playerActor == nullptr) return;

	Vector3D movementDir;
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


void terminateProgram()
{
	if (bWinsockLoaded) WSACleanup();
	glfwTerminate();
}


void updateActors(char* buffer, int bufferLen)
{

	// TODO: It is no longer true that the first actor received is the actor belonging to this client. This method must be reworked




	/* -- Schema -- 
	*  0  - 3  : id
	*  4  - 16 : location
	*  15 - 27 : rotation
	*/

	// Creating actor index and checking for invalid bufferLen
	unsigned int numActors = -1;
	float check = bufferLen / (float)sizeof(Actor); // BUG
	if (check - floor(check) == 0.f)
		numActors = (int)floor(check);
	if (numActors == -1)
	{
		std::cout << "Client::updateActors -- length of buffer not wholly divisible by sizeof(Actor)\n";
		return;
	}
	
	// -- Reading Actors -- //
	Actor* actor;
	for (unsigned int i = 0; i < numActors; i++)
	{
		unsigned int id = 0;
		Vector3D position;
		Vector3D rotation;
		memcpy(&id, buffer, sizeof(unsigned int));
		memcpy(&position, buffer + sizeof(unsigned int), sizeof(Vector3D));
		memcpy(&rotation, buffer + sizeof(unsigned int) + sizeof(Vector3D), sizeof(Vector3D));

		if (actorMap.count(id) == 0) // Actor not found
		{
			// Add actor to map
			Actor* newActor = new Actor(position, rotation, id);
			actorMap[id] = newActor;
			if (!playerActor) // First time receiving actor data?
				playerActor = newActor;
		}
		else // Actor found. Updating Actor data
		{
			actor = actorMap[id];
			actor->setPosition(position);
			actor->setRotation(rotation);
		}
		buffer += sizeof(Actor);
	}
}


void handleCommand(char* buffer, unsigned int bufferLen)
{
	switch (buffer[0]) 
	{
		case '0':// Connection Reply
		{
			playerActor = new Actor(*Actor::deserialize(buffer, bufferLen));
			break;
		}
	}
}