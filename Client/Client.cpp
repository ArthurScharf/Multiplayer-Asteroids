/**
* NOTES
* 
* Serializing client inputs to be sent to server
*	
* 
* 
*/









#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <WinSock2.h>
#include <WS2tcpip.h> // inetPtons
#include <tchar.h> // _T
#include <thread>
#include <map>
#include <set>

#include "Camera.h"

#include "../CommonClasses/Actor.h"
#include "../CommonClasses/Definitions.h"
#include "../CommonClasses/UDPSocket.h"
#include "../CommonClasses/Vector3D.h"
#include "../CommonClasses/Serialization/Serializer.h"

#pragma comment(lib, "ws2_32.lib") // What is this doing?

const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;


bool bWinsockLoaded = false;


// Camera
Camera camera( glm::vec3(0.f, 0.f, 100.f) ); // Camera Position
//Camera camera(0.f, 0.f, 100.f, 0.f, 1.f, 0.f, -90.f, 0.f);


UDPSocket sock;
sockaddr_in serverAddr;
bool bConnectedToServer = false;


/* There is a playerActorID and a playerActor pointer for the following reasons
* 
* Actors are spawned if the client receives actor replication and an actor is discovered
* that it hasn't yet seen.
* When a client connects, it is added to a list of clients on the server.
* Once the server decides to spawn the player's actor, it will send to that client the network
* id for that actor so the client can have it.
* Rather than require that we send actor data to all clients for each spawn, we instead
* simply replicate the newly created actor using the normal method; server cycle by server cycle.
* This means there exists a state where the client has a controlled actor ID, but doesn't
* yet have that actor to control.
* 
* NOTE: This feels convoluted. I should think about reworking this
*/

// A Map is used so we don't need to create an actor each time we parse actor data to see if that actor exists. We can just see if it's id has been placed in this map
std::map<unsigned int, Actor*> actorMap;
// Stores network ID for actor this client is controlling. Necessary bc a client can be connected without an actor
unsigned int playerActorID = -1;
// Pointer to actor being controlled by this client. Avoids repeated lookups using playerActorID
Actor* playerActor = nullptr;

const float playerSpeed = 25.f; // The rate at which the player changes position


float deltaTime = 0.f;
float lastFrame = 0.f;


const std::string gearFBX_path = "C:/Users/User/source/repos/Multiplayer-Asteroids/CommonClasses/FBX/Gear/Gear1.fbx";


// -- Forward Declaring Functions -- //
void processInput(GLFWwindow* window);
void terminateProgram();
void updateActors(char* buffer, int bufferLen);
void handleServerMessage(char* buffer, unsigned int bufferLen);


/* It is the job of the client's main code to organize and denote the structure
*  Of the serialized data being sent. In other words, it must
*  organize the serialized actors and instructions with the same custom protocol
*  that the server does
*/
// ---- CLIENT ---- //
int main()
{
	// ---------- Networking ---------- //
	sock.init(false);
	/* Properly formatting received IP address string and placing it in sockaddr_in struct */
	serverAddr.sin_family = AF_INET;
	std::cout << "Enter server IP address: ";
	wchar_t wServerAddrStr[INET_ADDRSTRLEN];
	std::wcin >> wServerAddrStr;
	PCWSTR str(wServerAddrStr);
	InetPtonW(AF_INET, str, &(serverAddr.sin_addr));
	serverAddr.sin_port = htons(6969);


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
		terminateProgram();
	}

	// OpenGL Settings
	glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS); // Explicitly stating the depth function to be used

	// Compiling GLSL Shaders 
	Shader shader(
		"C:/Users/User/source/repos/Multiplayer-Asteroids/CommonClasses/GLSL Shaders/Vertex.txt",
		"C:/Users/User/source/repos/Multiplayer-Asteroids/CommonClasses/GLSL Shaders/Fragment.txt"
	);


	// Main loop
	while (!glfwWindowShouldClose(window))
	{
		// Per-frame logic
		float currentFrame = static_cast<float>(glfwGetTime());
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		processInput(window);

		// -- Rendering Actors -- //
		glClearColor(0.1f, 0.1f, 0.1f, 1.f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		shader.use();

		glm::mat4 model = glm::mat4(1.f);
		glm::mat4 view = glm::mat4(1.f);
		glm::mat4 projection = glm::mat4(1.f);

		model = glm::translate(model, glm::vec3(0.f));
		shader.setMat4("model", model);
		view = camera.GetViewMatrix();
		shader.setMat4("view", view);
		projection = glm::perspective(glm::radians(camera.zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.f);
		shader.setMat4("projection", projection);
		for (auto iter = actorMap.begin(); iter != actorMap.end(); ++iter)
		{
			iter->second->Draw(shader);
		}

		// -- Sending Data to Server -- //
		char sendBuffer[1 + (MAX_ACTORS * sizeof(Actor))]; // I doubt command data will ever exceed this size
		// No player actor --> The server hasn't given us one yet; we're not connected
		if (!bConnectedToServer)
		{
			sendBuffer[0] = 'c'; // Connect
			sock.sendData(sendBuffer, 1, serverAddr);
		}


		// -- Receiving Data from Server -- // Receives data from server regardless of state
		int numBytesRead = 0;
		sockaddr_in recvAddr;
		char* recvBuffer{};
		recvBuffer = sock.recvData(numBytesRead, recvAddr);
		if (numBytesRead > 0)
		{
			handleServerMessage(recvBuffer, numBytesRead);
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
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);
	if (playerActor == nullptr) return; 

	// -- Camera Testing -- //
	//if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
	//	camera.position += glm::vec3(0.f, 1.f, 0.f)  * playerSpeed * deltaTime;
	//if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
	//	camera.position += glm::vec3(0.f, -1.f, 0.f) * playerSpeed * deltaTime;
	//if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
	//	camera.position += glm::vec3(1.f, 0.f, 0.f) * playerSpeed * deltaTime;
	//if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
	//	camera.position += glm::vec3(-1.f, 0.f, 0.f) * playerSpeed * deltaTime;
	//printf("%.4f, %.4f, %.4f", camera.position.x, camera.position.y, camera.position.z);
	
	Vector3D movementDir;
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		movementDir += Vector3D(0.f, 1.f, 0.f);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		movementDir += Vector3D(0.f, -1.f, 0.f);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		movementDir += Vector3D(1.f, 0.f, 0.f);
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		movementDir += Vector3D(-1.f, 0.f, 0.f);
	if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
		movementDir += Vector3D(0.f, 0.f, -1.f);
	if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
		movementDir += Vector3D(0.f, 0.f, 1.f);
	movementDir.Normalize();

	// TODO: modify to handle input bit and handle masking of inputs above
	char sendBuffer[sizeof(Vector3D) + 1]{ 0 }; // Schema: movement input, other keyboard like quitting or shooting
	sendBuffer[0] = 'r'; // 'r' --> actor replication
	memcpy(sendBuffer + 1, &movementDir, sizeof(Vector3D)); // Copying movement direction
	sock.sendData(sendBuffer, sizeof(Vector3D) + 1, serverAddr);
	//playerActor->setPosition(playerActor->getPosition() + (movementDir * playerSpeed * deltaTime));
}



int alternatingModelIndex = 0; // TEMP
void updateActors(char* buffer, int bufferLen)
{

	// TODO: Actor's must have a mesh instantiated when they're created. Data sent across must indicate the type of actor
	//       Alternatively, we send spawn and destroy messages, which contain all the required actor data, including the type of actor


	/* -- Schema -- 
	*  0  - 3  : id
	*  4  - 16 : location
	*  15 - 27 : rotation
	*/

	// Creating numActors & Checking for invalid bufferLen
	unsigned int numActorsReceived = -1;
	float check = bufferLen / (float)sizeof(Actor);
	if (check - floor(check) == 0.f)
		numActorsReceived = (int)floor(check);
	if (numActorsReceived == -1)
	{
		std::cout << "Client::updateActors -- length of buffer not wholly divisible by sizeof(Actor)\n";
		return;
	}
	
	//std::cout << numActorsReceived << std::endl;

	// -- Reading Actors -- //
	Actor* actor;
	for (unsigned int i = 0; i < numActorsReceived; i++)
	{
		unsigned int id;
		Vector3D position;
		Vector3D rotation;
		memcpy(&id, buffer, sizeof(unsigned int));
		memcpy(&position, buffer + sizeof(unsigned int), sizeof(Vector3D));
		memcpy(&rotation, buffer + sizeof(unsigned int) + sizeof(Vector3D), sizeof(Vector3D));

		if (actorMap.count(id) == 0) // Actor not found
		{
			// Add actor to map
			actor = new Actor(position, rotation, id);
			//if ((alternatingModelIndex % 2) == 0)
			//{
			//	actor->InitializeModel("C:/Users/User/source/repos/Multiplayer-Asteroids/CommonClasses/FBX/Gear/Gear1.fbx");// TEMP			
			//}
			//else
			//{
			//	actor->InitializeModel("C:/Users/User/source/repos/Multiplayer-Asteroids/CommonClasses/FBX/chair/chair.fbx");// TEMP			
			//}
			actor->InitializeModel("C:/Users/User/source/repos/Multiplayer-Asteroids/CommonClasses/FBX/Gear/Gear1.fbx");// TEMP			
			alternatingModelIndex++;
			actorMap[id] = actor;

		}
		else // Actor found. Updating Actor data
		{
			actor = actorMap[id];
			actor->setPosition(position);
			actor->setRotation(rotation);
			//std::cout << actor->toString() << std::endl;
		}


		if (id == playerActorID && actor != playerActor)
		{
			printf("Client::updateActors -- Updating playerActor");
			playerActor = actor;
		}
		buffer += sizeof(Actor);
	}
}


void handleServerMessage(char* buffer, unsigned int bufferLen)
{
	switch (buffer[0]) 
	{
		case 'c':	// Connection Reply
		{
			std::cout << "Successfully connected\n";
			bConnectedToServer = true;
			break;
		}
		case 'r':	// Replicating
		{
			/* Updates actor states to match server data.
			*  Spawns actors that were sent to client, but don't yet exist
			*/
			// std::cout << "Replicating Actor Data\n";
			updateActors(++buffer, --bufferLen);
			break;
		}
		case 'i':	// Receiving Controlled Actor ID
		{
			memcpy(&playerActorID, ++buffer, sizeof(unsigned int));
			std::cout << "Receiving playerActorID: " << playerActorID << std::endl;
			break;
		}
	}
}


void terminateProgram()
{
	if (bWinsockLoaded) WSACleanup();
	glfwTerminate();
}


