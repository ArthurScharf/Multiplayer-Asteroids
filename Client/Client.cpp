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
char sendBuffer[1 + (MAX_ACTORS * sizeof(Actor))];
char* recvBuffer{};
int numBytesRead = -1; // TODO: This might be a duplicate of another variable



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



/*
KEY   : Actor Network ID
VAlUE : The corresponding actor
*/
std::map<unsigned int, Actor*> actorMap;
// Stores network ID for actor this client is controlling. Necessary bc a client can be connected without an actor
unsigned int playerActorID = -1;
// Pointer to actor being controlled by this client. Avoids repeated lookups using playerActorID
Actor* playerActor = nullptr;




bool bFireButtonPressed = false;



unsigned int nextProxyID = 0;
// Key: proxyID / Value: Proxy
std::map<unsigned int, Actor*> unlinkedProxies;






/* Stores the states created by the client at each fixed update.
 * Used with incoming states from the server to compare for discrepencies */
CircularBuffer stateBuffer;






// -- Time -- //
float deltaTime = 0.f;
float lastFrame = 0.f;
float updatePeriod = 1.f / 20.f; // Verbose to allow easy editing. Should be properly declared later 20.f
float elapsedTimeSinceUpdate = 0.f;
unsigned int stateSequenceID = 0;




const std::string gearFBX_path = "C:/Users/User/source/repos/Multiplayer-Asteroids/CommonClasses/FBX/Gear/Gear1.fbx";




// -- Forward Declaring Functions -- //
void initiateConnection(char* sendBuffer, char* recvBuffer, int& numBytesRead);
void processInput(GLFWwindow* window);
void spawnProjectile();
void moveActors(float deltaTime); // Moves the actors locally. Lower priority actor update than the fixed updates
void replicateState(char* buffer, int bufferLen);
void readRecvBuffer();
void handleMessage(char* buffer, unsigned int bufferLen);
void terminateProgram();



int main()
{
	// -- Initiating Connection with Server -- //
	initiateConnection(sendBuffer, recvBuffer, numBytesRead);

	// -- OpenGL -- //
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

	// -- Loading Models -- //
	Actor::loadModelCache(); // Initializes model cache so blueprinted actors can be created


	/*
	// -- Retrieving Server's Most Recently Simulated Timestep -- //
	for (int i = 0; i < 5; i++) // Arbitrary number of attempts
	{
		std::cout << "Sending MSG_TSTEP" << std::endl;
		sendBuffer[0] = MSG_TSTEP;
		sock.sendData(sendBuffer, 1, serverAddr);

		Sleep(250); // Reasonable amount of time

		readRecvBuffer();

		if (stateSequenceID != 0) break;
	}
	// Program exiting is simple way to avoid problems. We assume that LAN will usually return 1 of 5 attempts
	if (stateSequenceID == 0)
	{
		std::cout << "ERROR::main / Retrieving Server stateSequenceID -- Retrieved ID == 0" << std::endl;
		//terminateProgram();
	}
	*/



	/* glfwGetTime returns time since glfw was initialized. We want to only begin measure from the 
	*  point at which the main loop began to run.
	*/
	lastFrame = static_cast<float>(glfwGetTime());

	// ---------- Main loop ---------- //
	while (!glfwWindowShouldClose(window))
	{

		// ---- Per-frame logic ---- //
		float currentFrame = static_cast<float>(glfwGetTime());
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;
		elapsedTimeSinceUpdate += deltaTime;

		// ---- Input Handling ---- //
		processInput(window);
		moveActors(deltaTime);

		// ---- Rendering Actors ---- //
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
			iter->second->Draw(shader); // Actor handles setting the position offset for shader
		}
		for (auto iter = unlinkedProxies.begin(); iter != unlinkedProxies.end(); iter++)
		{
			iter->second->Draw(shader);
		}


		// ---- Fixed Frequency Update ---- //
		if (elapsedTimeSinceUpdate >= updatePeriod)
		{
			elapsedTimeSinceUpdate -= updatePeriod;
			stateSequenceID++;

			// std::cout << "Fixed update " << stateSequenceID << std::endl;


			std::cout << "num actors : " << actorMap.size() << " , " << "num Proxies : " << unlinkedProxies.size() << std::endl;


			// -- Creating New State -- //
			std::vector<Actor*> actors;
			for (auto iter = actorMap.begin(); iter != actorMap.end(); iter++)
			{
				actors.push_back(iter->second);
			}
			GameState* gameState = new GameState(stateSequenceID, actors);
			stateBuffer.append(gameState);

			if (!playerActor) continue; // BUG: Remove this to see bug

			char sendBuffer[1 + sizeof(ActorNetData)]{0};
			sendBuffer[0] = MSG_REP;

			ActorNetData data = playerActor->toNetData();
			memcpy(sendBuffer + 1, &data, sizeof(ActorNetData));
			sock.sendData(sendBuffer, 1 + sizeof(ActorNetData), serverAddr);

		}//~ Fixed Update

		readRecvBuffer();
		glfwPollEvents();
		glfwSwapBuffers(window);
	}//~ Main Loop



	// TEST
	sendBuffer[0] = MSG_EXIT;
	sock.sendData(sendBuffer, 1, serverAddr);

	terminateProgram();

	// Prevents window from closing too quickly. Good so I can see print statements
	char temp[200];
	std::cin >> temp;

	return 0;
};

/*
* Takes input from user for game server IP.
* Sends connect 'c'  message then waits for reply.
* If reply isn' received in 1/4 s, take input from user again
*/
void initiateConnection(char* sendBuffer, char* recvBuffer, int& numBytesRead)
{
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
		for (int i = 0; i < 5; i++)
		{
			sendBuffer[0] = MSG_CONNECT; // Connect
			sock.sendData(sendBuffer, 1, serverAddr);

			Sleep(250); // quarter second seems reasonable

			recvBuffer = sock.recvData(numBytesRead, serverAddr);

			if (recvBuffer[0] = MSG_CONNECT)
			{
				bIsConnectedToServer = true;
				break;
			}
		}
	}
}


void processInput(GLFWwindow* window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);
	if (playerActor == nullptr) return; 
	
	// -- Movement -- //
	Vector3D moveDirection(0.f);
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
	{
		moveDirection += Vector3D(0.f, 1.f, 0.f);
	}
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
	{
		moveDirection += Vector3D(0.f, -1.f, 0.f);
	}
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
	{
		moveDirection += Vector3D(1.f, 0.f, 0.f);
	}
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
	{
		moveDirection += Vector3D(-1.f, 0.f, 0.f);
	}
	if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
	{
		moveDirection += Vector3D(0.f, 0.f, -1.f);
	}
	if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
	{
		moveDirection += Vector3D(0.f, 0.f, 1.f);
	}
	moveDirection.Normalize();
	playerActor->setMoveDirection(moveDirection);


	// -- Actions -- //
	if (glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS)
	{
		if (!bFireButtonPressed)
		{
			spawnProjectile();
			bFireButtonPressed = true;
		}
	}
	else if (glfwGetKey(window, GLFW_KEY_J) == GLFW_RELEASE)
	{
		bFireButtonPressed = false;
	}
}


void spawnProjectile()
{
	if (!playerActor) return;

	printf("Client::spawnProjectile\n");

	/* -- NOTE -- 
	I didn't feel like this single case for spawning a proxy deserved it's own constructor, as this would
	present a more confusing header for actor. Thus, I'm setting things manually here, since this process should
	only ever be done once */

	// ---- Constructing Unlinked Proxy ---- //
	Actor* projectile = new Actor(
		playerActor->getPosition() + (playerActor->getRotation() * 100.f),
		playerActor->getRotation(),
		ABI_Projectile,
		true,
		nextProxyID
	);
	unlinkedProxies.insert({ nextProxyID, projectile });

	// ---- Sending data to server ---- //
	char buffer[sizeof(NetworkSpawnData)];
	NetworkSpawnData data;
	data.dummyActorID = nextProxyID;
	data.simulationStep = stateSequenceID;
	data.networkedActorID = 0; // The proxy's ID doesn't matter to the server
	memcpy(buffer, &data, sizeof(NetworkSpawnData));
	sock.sendData(buffer, 1 + sizeof(NetworkSpawnData), serverAddr);

	nextProxyID++;
}


void moveActors(float deltaTime)
{
	// ---- Moving Actors ---- //
	for (auto iter = actorMap.begin(); iter != actorMap.end(); iter++)
	{
		Actor* actor = iter->second;
		actor->addToPosition(actor->getMoveDirection() * actor->getMoveSpeed() * deltaTime);
		
		// TODO: Implement propery approach to limiting actor movement. Different types of actors will handle the edge of the screen differently
		
		// -- X-boundary -- //
		if (actor->getPosition().x > 62.f) // I have no idea why this value is edge of the screen
		{
			actor->setPosition(Vector3D(62.f, actor->getPosition().y, actor->getPosition().z));
		}
		else if (actor->getPosition().x < -62.f)
		{
			actor->setPosition(Vector3D(-62.f, actor->getPosition().y, actor->getPosition().z));
		}
		// -- Y-boundary -- //
		if (actor->getPosition().y > 50.f)
		{
			actor->setPosition(Vector3D(actor->getPosition().x, 50.f, actor->getPosition().z));
		}
		else if (actor->getPosition().y < -50.f)
		{
			actor->setPosition(Vector3D(actor->getPosition().x, -50.f, actor->getPosition().z));
		}
	}


	// TODO: Why do I have a separate collection for proxies? Wouldn't it make more sense to identify them some other way?

	// ---- Moving Unlinked Proxies ---- //
	for (auto iter = unlinkedProxies.begin(); iter != unlinkedProxies.end(); iter++)
	{
		iter->second->addToPosition(iter->second->getMoveDirection() * iter->second->getMoveSpeed() * deltaTime);
	}
}


void handleMessage(char* buffer, unsigned int bufferLen)
{
	switch (buffer[0]) 
	{
		case MSG_CONNECT:	// Connection Reply
		{
			printf("handleMessage / MSG_CONNECT\n");
			bIsConnectedToServer = true;
			break;
		}
		case MSG_TSTEP:	// Receive current simulation step
		{
			memcpy(&stateSequenceID, buffer + 1, sizeof(unsigned int));
			stateSequenceID += 5;
			printf("handleMessage / MSG_TSTEP -- Received State Sequence ID : %d\n", stateSequenceID);
			break;
		}
		case MSG_REP:	// Replicating
		{
			// std::cout << "handleMessage / MSG_REP" << std::endl;
			/* Updates actor states to match server data.
			*  Spawns actors that were sent to client, but don't yet exist
			*/
			replicateState(++buffer, --bufferLen); // TODO: Fixed update actors shouldn't be here. It should always be called and check for missing states when comparing
			break;
		}
		case MSG_ID:	// Receiving Controlled Actor ID
		{
			memcpy(&playerActorID, ++buffer, sizeof(unsigned int));
			printf("handleMessage / MSG_ID : playerActorID: %d\n", playerActorID);
			break;
		}
		case MSG_SPAWN:
		{
			printf("handleMessage / MSG_SPAWN\n");

			/* Reply to Spawn Request
			*  Links locally spawned dummy with authoritative server copy
			*/
			NetworkSpawnData data;
			memcpy(&data, buffer, sizeof(NetworkSpawnData));

			/*
			unlinked proxy becomes a normal actor. This avoids the overhead cost of constructing a new actor when required
			*/
			Actor* linkedActor = unlinkedProxies[data.dummyActorID];
			linkedActor->setId(data.networkedActorID);
			actorMap[data.networkedActorID] = linkedActor;
			unlinkedProxies.erase(data.dummyActorID);

			printf("  (dummyActorID, networkedActorID) : (%d , %d)\n", data.dummyActorID, data.networkedActorID);
			break;
		}
	}
}


void replicateState(char* buffer, int bufferLen)
{
	// std::cout << "Client::replicateState\n";

	// -- Creating numActorsReceived & Checking for invalid bufferLen -- //
	unsigned int numActorsReceived = -1;
	float check = (bufferLen - sizeof(unsigned int)) / (float)sizeof(ActorNetData);
	if (check - floor(check) == 0.f)
		numActorsReceived = (int)floor(check);
	if (numActorsReceived == -1)
	{
		printf("Client::replicateState -- length of buffer not wholly divisible by sizeof(ActorNetData)");
		return;
	}


	// -- Creating and Copying to stateSequenceID -- //
	unsigned int stateSequenceID = -1;
	memcpy(&stateSequenceID, buffer, sizeof(unsigned int));
	buffer += sizeof(unsigned int);


	// -- Reading Actors -- //
	Actor* actor;
	for (unsigned int i = 0; i < numActorsReceived; i++)
	{
		ActorNetData netData;
		memcpy(&netData, buffer, sizeof(netData));
		
		// -- New Actor Encountered -- //
		if (actorMap.count(netData.id) == 0)
		{
			/*
			* ---- DANGER ----
			* if replication happens BEFORE spawn reply can return, We'll have two versions of the actor floating around
			*/

			// -- Add actor to map -- //
			actor = new Actor(netData.Position, netData.Rotation, netData.blueprintID, true, netData.id);
			actorMap[netData.id] = actor;
		}
		else // -- Actor found. Updating Actor data -- //
		{
			actor = actorMap[netData.id];
			actor->setPosition(netData.Position);
			actor->setRotation(netData.Rotation);
			actor->setMoveDirection(netData.moveDirection);
			actor->setMoveSpeed(netData.moveSpeed);
		}
		// -- Checking to see if we've encountered the player's controlled actor -- //
		if (netData.id == playerActorID && actor != playerActor)
		{
			printf("Client::replicateState -- Updating playerActor\n");
			playerActor = actor;
		}
		buffer += sizeof(ActorNetData);
	}//~ Handling single actor of those received



	// -- Creating State and Checking for equality -- //
	std::vector<Actor*> temp;
	for (auto iter = actorMap.begin(); iter != actorMap.end(); iter++)
	{
		temp.push_back(iter->second);
	}
	GameState* state = new GameState(stateSequenceID, temp);
	GameState* clientState = stateBuffer.getStateBySequenceId(stateSequenceID);
	if (!clientState)
	{
		// std::cout << "replicateState -- !clientState" << std::endl;
	}
	


	if ((clientState == state) == false)
	{
		// std::cout << "replicateState -- Not equivalent" << std::endl;
	}
	

	return;


	if (!stateBuffer.contains(state))
	{
		// TODO: Reconcile
	}
	delete state;
}




/*
Reads NUM_PACKETS_PER_CYCLE messages from the receive buffer.
Anything that isnt MSG_REP, will be processed immediately.
Only the most recently received MSG_REP, within NUM_PACKETS_PER_CYCLE, will be processed.
*/
void readRecvBuffer()
{
	// ----  Receiving Data from Server ----  // Receives data from server regardless of state.
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
		if (*tempBuffer != MSG_REP)
		{
			handleMessage(tempBuffer, tempBufferLen);
			continue;
		}
		recvBuffer = tempBuffer;
		recvBufferLen = tempBufferLen;
	}
	if (recvBufferLen > 0)
	{
		handleMessage(recvBuffer, recvBufferLen);
	}
}


void terminateProgram()
{
	if (bWinsockLoaded) WSACleanup();
	glfwTerminate();
}


