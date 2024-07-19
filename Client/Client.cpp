/**
* NOTES
* 
* Serializing client inputs to be sent to server
*	
* 
* 
*/



/*
AAAA : modify to handle input bit and handle masking of inputs above

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




#include "GameState.h"
#include "CircularBuffer.h"



#pragma comment(lib, "ws2_32.lib") // What is this doing?

const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

const unsigned int NUM_PACKETS_PER_CYCLE = 10;


bool bWinsockLoaded = false;


// Camera
Camera camera( glm::vec3(0.f, 0.f, 100.f) ); // Camera Position
//Camera camera(0.f, 0.f, 100.f, 0.f, 1.f, 0.f, -90.f, 0.f);


UDPSocket sock;
sockaddr_in serverAddr;
bool bIsConnectedToServer = false;


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


// ---- Player Character ---- //
// A Map is used so we don't need to create an actor each time we parse actor data to see if that actor exists. We can just see if it's id has been placed in this map
std::map<unsigned int, Actor*> actorMap;
// Stores network ID for actor this client is controlling. Necessary bc a client can be connected without an actor
unsigned int playerActorID = -1;
// Pointer to actor being controlled by this client. Avoids repeated lookups using playerActorID
Actor* playerActor = nullptr;
Vector3D movementDir;
const float playerSpeed = 70.f; // The rate at which the player changes position



/* Stores the states created by the client at each fixed update.
 * Used with incoming states from the server to compare for discrepencies
 */
CircularBuffer stateBuffer;



// -- Time -- //
float deltaTime = 0.f;
float lastFrame = 0.f;
float updatePeriod = 1.f / 4.f; // Verbose to allow easy editing. Should be properly declared later // 20.f; 
float elapsedTimeSinceUpdate = 0.f;
unsigned int stateSequenceId = 0;


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
	char sendBuffer[1 + (MAX_ACTORS * sizeof(Actor))];
	char* recvBuffer{};
	int numBytesRead = -1; // TODO: This might be a duplicate of another variable

	// ---- Establishing Connection ---- //
	sock.init(false);
	/* Properly formatting received IP address string and placing it in sockaddr_in struct */
	serverAddr.sin_family = AF_INET;

	while (!bIsConnectedToServer)
	{
		std::cout << "Enter server IP address: ";
		wchar_t wServerAddrStr[INET_ADDRSTRLEN];
		std::wcin >> wServerAddrStr;
		PCWSTR str(wServerAddrStr);
		InetPtonW(AF_INET, str, &(serverAddr.sin_addr));
		serverAddr.sin_port = htons(6969);
		// -- Sending Connection Message -- //
		// Attempt to connect several times
		for (int i = 0; i < 4; i++)
		{
			sendBuffer[0] = 'c'; // Connect
			sock.sendData(sendBuffer, 1, serverAddr);

			Sleep(250); // quarter second seems reasonable

			sockaddr_in _; // TODO: Code smell
			recvBuffer = sock.recvData(numBytesRead, _);
			if (numBytesRead > 0 && *recvBuffer == 'c')
			{
				bIsConnectedToServer = true;
				stateSequenceId = *(unsigned int*)(recvBuffer + 1);
				stateSequenceId += 5; // TODO: Arbitrary guess to get the client ahead. More elegent solution in the future
				break;
			}
		}
	}




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


	// ---------- Main loop ---------- //
	while (!glfwWindowShouldClose(window))
	{

		// -- Per-frame logic -- //
		float currentFrame = static_cast<float>(glfwGetTime());
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;
		elapsedTimeSinceUpdate += deltaTime;

		// -- Input Handling -- //
		movementDir.x = movementDir.y = movementDir.z = 0.f; // Reset for this frame
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


		// ---- Fixed Update ---- //
		if (elapsedTimeSinceUpdate >= updatePeriod)
		{
			elapsedTimeSinceUpdate -= updatePeriod;
			stateSequenceId++;

			std::cout << stateSequenceId << std::endl;

			// TODO: AAAA
			char sendBuffer[sizeof(Vector3D) + 1]{ 0 }; // Schema: movement input, other keyboard like quitting or shooting
			sendBuffer[0] = 'r'; // 'r' --> actor replication
			memcpy(sendBuffer + 1, &movementDir, sizeof(Vector3D)); // Copying movement direction
			sock.sendData(sendBuffer, sizeof(Vector3D) + 1, serverAddr);
		}


		// -- Receiving Data from Server -- // Receives data from server regardless of state
		char* tempBuffer{};
		int recvBufferLen = 0;
		int tempBufferLen = 0;
		sockaddr_in recvAddr; // This is never used on purpose. Code smell
		for (int i = 0; i < NUM_PACKETS_PER_CYCLE; i++)
		{
			tempBuffer = sock.recvData(tempBufferLen, recvAddr);
			if (tempBufferLen == 0)
			{
				printf("client::reading packets -- read all packets");
				break;
			}
			if (*tempBuffer != 'r')
			{
				handleServerMessage(tempBuffer, tempBufferLen);
				continue;
			}
			recvBuffer = tempBuffer;
			recvBufferLen = tempBufferLen;
		}
		if (recvBufferLen > 0)
		{
			handleServerMessage(recvBuffer, recvBufferLen);
		}


		glfwPollEvents();
		glfwSwapBuffers(window);
	}//~ Main Loop

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


	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
	{
		movementDir += Vector3D(0.f, 1.f, 0.f);
	}
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
	{
		movementDir += Vector3D(0.f, -1.f, 0.f);
	}
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
	{
		movementDir += Vector3D(1.f, 0.f, 0.f);
	}
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
	{
		movementDir += Vector3D(-1.f, 0.f, 0.f);
	}
	if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
	{
		movementDir += Vector3D(0.f, 0.f, -1.f);
	}
	if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
	{
		movementDir += Vector3D(0.f, 0.f, 1.f);
	}
	movementDir.Normalize();
	
	// TODO: Allow client side simulation
	// playerActor->setPosition(playerActor->getPosition() + (movementDir * 70.f * deltaTime));
}


void handleServerMessage(char* buffer, unsigned int bufferLen)
{
	switch (buffer[0]) 
	{
		case 'c':	// Connection Reply
		{
			std::cout << "Successfully connected\n";
			bIsConnectedToServer = true;
			stateSequenceId = *(unsigned int*)(buffer + 1);
			std::cout << "Received Simulation Step: " << stateSequenceId << std::endl;
			stateSequenceId += 5; // TODO: Hardcoded solution to getting the client ahead. Should later be replaced with a more elegent solution
			break;
		}
		case 'r':	// Replicating
		{
			/* Updates actor states to match server data.
			*  Spawns actors that were sent to client, but don't yet exist
			*/
			updateActors(++buffer, --bufferLen);
			break;
		}
		case 'i':	// Receiving Controlled Actor ID
		{
			memcpy(&playerActorID, ++buffer, sizeof(unsigned int));
			std::cout << "Receiving playerActorID: " << playerActorID << std::endl;
			break;
		}
		case 't':
		{
			std::cout << "client::handleServerMessage/t" << std::endl;
			//currTime = glfwGetTime();
			//diff = currTime - lastTime;
			//std::cout << diff << std::endl;
			break;
		}
	}
}


void updateActors(char* buffer, int bufferLen)
{
	// TODO: Actor's must have a mesh instantiated when they're created. Data sent across must indicate the type of actor
	//       Alternatively, we send spawn and destroy messages, which contain all the required actor data, including the type of actor
	/* -- Schema --
	*  0  - 3  : stateSequenceId
	*  (N + ...)
	*  0  - 3  : id
	*  4  - 16 : location
	*  15 - 27 : rotation
	*/


	// -- Creating numActors & Checking for invalid bufferLen -- //
	unsigned int numActorsReceived = -1;
	float check = bufferLen / (float)sizeof(Actor);
	if (check - floor(check) == 0.f)
		numActorsReceived = (int)floor(check);
	if (numActorsReceived == -1)
	{
		std::cout << "Client::updateActors -- length of buffer not wholly divisible by sizeof(Actor)\n";
		return;
	}

	// -- Creating and Copying to stateSequenceId -- //
	unsigned int stateSequenceId = -1;
	memcpy(&stateSequenceId, buffer, sizeof(unsigned int));
	buffer += sizeof(unsigned int);

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

		// -- Actor not found -- //
		if (actorMap.count(id) == 0) 
		{
			// -- Add actor to map -- //
			actor = new Actor(position, rotation, id);
			actor->InitializeModel("C:/Users/User/source/repos/Multiplayer-Asteroids/CommonClasses/FBX/Gear/Gear1.fbx");
			actorMap[id] = actor;

			// -- GameState -- //
		}
		else // -- Actor found. Updating Actor data -- //
		{
			actor = actorMap[id];
			if (actor->getPosition() != position)
			{
				printf("Client::main/reading_actors -- Receiving Replication\n");
			}
			actor->setPosition(position);
			actor->setRotation(rotation);
		}

		if (id == playerActorID && actor != playerActor)
		{
			printf("Client::updateActors -- Updating playerActor\n");
			playerActor = actor;
		}
		buffer += sizeof(Actor);
	}
}


void terminateProgram()
{
	if (bWinsockLoaded) WSACleanup();
	glfwTerminate();
}


