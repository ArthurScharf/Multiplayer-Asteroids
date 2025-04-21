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
//#include <queue>
#include <bitset> // For testing. Remove later Should use pre-processor commands with DEBUG to automate this

#include "Camera.h"

#include "../CommonClasses/Actor.h"
#include "../CommonClasses/Definitions.h"
#include "../CommonClasses/UDPSocket.h"
#include "../CommonClasses/Vector3D.h"
#include "../CommonClasses/Serialization/Serializer.h"




#include "GameState.h"
#include "CircularBuffer.h"




#pragma comment(lib, "ws2_32.lib") // What is this doing?


// -- Pre-processor Commands -- //			<---- These are used to include/disclude features.
#define PROXIES true


// ---- Rendering ----- //
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;
bool bWinsockLoaded = false;
GLFWwindow* window;
Camera camera( glm::vec3(0.f, 0.f, 100.f) ); // Camera Position
//Camera camera(0.f, 0.f, 100.f, 0.f, 1.f, 0.f, -90.f, 0.f);
Shader* shader;
// bool bRenderingInitialized = false; // I think this was used for debugging at one point




// ----- Networking ----- //
UDPSocket sock;
sockaddr_in serverAddr;
bool bIsConnectedToServer = false;
char sendBuffer[1 + (MAX_ACTORS * sizeof(Actor))];
char* recvBuffer{};
int numBytesRead = -1; // TODO: This might be a duplicate of another variable
const unsigned int NUM_PACKETS_PER_CYCLE = 10; // 10

/*
* Byte used to ID owned actors over the network
*/
int clientNetworkID;


// -- Input Request -- //
unsigned int nextInputRequestID = 0; // NOTE: unsigned integers wrap around when incremented past their max. Ideal for IDs

//std::queue<ClientInputRequest*> storedInputRequests;
std::vector<ClientInputRequest*> storedInputRequests;

ServerGameState lastServerGameState;



// ----- Actors ----- //

/*
* The next ID the client believes the server will use when the client
* asks the server to create an actor it owns, predicting that actors ID.
* The server should create an actor with the same idea.
* When replicate state is received, the client looks to replace delete any proxy with an ID that belongs to
* it and instantiate a new actor using the data being received from the server
*/
unsigned int nextPredictedOwnedActorNetworkID = 0;

// KEY : Actor Network ID	| VALUE : The corresponding actor 
std::map<unsigned int, Actor*> actorMap;
// Stores network ID for actor this client is controlling. Necessary bc a client can be connected without an actor
unsigned int controlledActorID;
bool bPlayerActorIDSet = false;
// Pointer to actor being controlled by this client. Avoids repeated lookups using controlledActorID
Actor* playerActor = nullptr;

/* ! DEPRECATED	!
* TODO: Remove this
*/
unsigned int nextProxyID = 0;
// Key: proxyID / Value: Proxy
std::map<unsigned int, Actor*> unlinkedProxies;


// ----- Input ----- //
bool bFireButtonPressed  = false;
bool bTestButtonPressed  = false;
bool bReadyButtonPressed = false;



/* ---- Remote Procedure Calls ----

KEY : Simulation Step
VAL : Remote Procedure Calls

Each value is a list of remote procedure calls to be made during a simulation step (Key).
*/
std::map<unsigned int, std::vector<std::function<void()>>> RemoteProcedureCalls;


/* Used when exiting to close the main loop and exit the program */
bool bRunMainLoop = true;


/* Stores the states created by the client at each fixed update.
 * Used with incoming states from the server to compare for discrepencies */
CircularBuffer stateBuffer; // TODO: The class needs reworking




// -- Time -- //
float deltaTime = 0.f;
float lastFrame = 0.f;
unsigned int stateSequenceID = 0;
float tickPeriod = 1.f / 60.f;
float elapsedTimeSinceUpdate = 0.f; // Seconds


// -- Testing -- //
#if PROXIES 
float spawnTimeStart = 0.f;
float spawnLatencySeconds = 0.f;
bool bShouldPrintTime = true;
#endif



// ----- Function Declarations ----- //
void initiateConnectionLoop(); // char* sendBuffer, char* recvBuffer, int& numBytesRead
void waitingForGameToStartLoop();
void playingGameLoop();


// Initializes rendering for program, including creating a window
int initRendering(); 
/* Renders the game to the window */
void Render();

char processInput(GLFWwindow* window);
void spawnProjectile();
void destroyActor(unsigned int id);
void moveActors(float deltaTime); // Moves the actors locally. Lower priority actor update than the fixed updates
void replicateState(char* buffer, int bufferLen);
void replicateState(ServerGameState& state);
void clientReconciliation();
void readRecvBuffer();
char handleMessage(char* buffer, unsigned int bufferLen); // returns message that was handled
void cleanup();









/*
* CS_InitializingConnection : Allows user to specify server ip. Sends connect message and waits for reply before moving to next state
* CS_WaitingForGameStart    : Allows user to send ready message. Cannot send ready message until client has controlled actor ID
* CS_PlayingGame            : User sends input requests to the server, performs client-side prediction, client-reconciliation, and renders
* CS_Exit					: Cleans up the program and exits
*/
enum ClientState
{
	CS_InitializingConnection,
	CS_WaitingForGameStart,
	CS_PlayingGame,
	CS_Exit
};
ClientState currentState;




int main()
{
	currentState = CS_InitializingConnection;

	while (bRunMainLoop)
	{
		switch (currentState)
		{
		case CS_InitializingConnection:
		{
			printf("Connecting to Server\n");
			initiateConnectionLoop();
			break;
		}
		case CS_WaitingForGameStart:
		{
			//if (!bRenderingInitialized)
			//	initRendering();
			initRendering();
			waitingForGameToStartLoop();
			break;
		}
		case CS_PlayingGame:
		{
			printf("Beginning Game\n");
			playingGameLoop();
			break;
		}
		case CS_Exit:
		{
			printf("Exiting\n");
			bRunMainLoop = false;
			break;
		}
		}//~ Switch
	}//~ While

	// So console doesn't immediately close
	sendBuffer[0] = MSG_EXIT;
	sock.sendData(sendBuffer, 1, serverAddr);

	cleanup();

	// Prevents window from closing too quickly. Good so I can see print statements
	char temp[200];
	std::cin >> temp;

	return 0;
};



void initiateConnectionLoop() // char* sendBuffer, char* recvBuffer, int& numBytesRead
{
	// ---- Establishing Connection ---- //
	sock.init(false);
	sock.setRecvBufferSize(2000); // TODO: Arbitrary. Should be chosen more intelligently
	/* Properly formatting received IP address string and placing it in sockaddr_in struct */
	serverAddr.sin_family = AF_INET;

	while (currentState == CS_InitializingConnection)
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
			// -- Sending connect message -- //
			sendBuffer[0] = MSG_CONNECT;
			sock.sendData(sendBuffer, 1, serverAddr);
			
			Sleep(500); 

			// -- Receiving server reply -- //
			recvBuffer = sock.recvData(numBytesRead, serverAddr);
			if (numBytesRead > 0)
			{
				if (handleMessage(recvBuffer, numBytesRead) == MSG_CONNECT) return;
			}
		}//~ for i
	}//~ While
}

void waitingForGameToStartLoop()
{
	printf("Space bar to ready up\n");

	while (currentState == CS_WaitingForGameStart)
	{
		// Can send MSG_STRTGM to server
		processInput(window);

		Render();

		// -- Polling Events & Swapping Buffers -- //
		glfwPollEvents();
		glfwSwapBuffers(window);

		// -- Receiving Messages -- //
		recvBuffer = sock.recvData(numBytesRead, serverAddr);
		if (numBytesRead == 0) continue;
		handleMessage(recvBuffer, numBytesRead);
	}//~ while
}

void playingGameLoop()
{
	/* glfwGetTime returns time since glfw was initialized. We want to only begin measure from the
	*  point at which the main loop began to run.
	*/
	lastFrame = static_cast<float>(glfwGetTime());
	char inputThisFrame = 0b00000000;

	while (currentState == CS_PlayingGame)
	{
		//-------------------------//
		//---- Per-frame logic ----//
		//-------------------------//
		if (storedInputRequests.size() > 5000)
		{
			printf("ERROR: stored input Requests exceeded limit. Terminating Program\n");
			currentState = CS_Exit;
			return;
		}

		float currentFrame = static_cast<float>(glfwGetTime());
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;
		elapsedTimeSinceUpdate += deltaTime;


		//-- Process, Apply, & Store Input for This Frame
		inputThisFrame = processInput(window);


		//--------------------------------//
		//---- Fixed Frequency Update ----//
		//--------------------------------//
		if (elapsedTimeSinceUpdate >= 1 / 60.f)
		{	// Locking to 60 FPS
			elapsedTimeSinceUpdate -= 1 / 60.f;

			//-- Storing & Sending Input Data
			ClientInputRequest* inputData = new ClientInputRequest();
			inputData->inputRequestID = nextInputRequestID++; // Reminder that the increment will wrap this around
			inputData->inputString = inputThisFrame;
			inputData->deltaTime = deltaTime;
			storedInputRequests.push_back(inputData);
			memcpy(sendBuffer, inputData, sizeof(ClientInputRequest));
			sock.sendData(sendBuffer, sizeof(ClientInputRequest), serverAddr);
		}//~ FFU

		//-- Moving Actors
		moveActors(deltaTime);

		//-- Rendering
		Render();

		readRecvBuffer();
		glfwPollEvents();
		glfwSwapBuffers(window);
	}//~ Main Loop
}





int initRendering()
{
	// -- OpenGL -- //
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// Creating the window 
	window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Asteroids", NULL, NULL);
	if (window == NULL)
	{
		std::cout << "main(): Failed to create window" << std::endl;
		glfwTerminate();
		return 1;
	}
	glfwMakeContextCurrent(window); // Why are we doing this again?
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	// Checking to see if GLAD loaded
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		cleanup();
	}
	// OpenGL Settings
	glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS); // Explicitly stating the depth function to be used


	// Compiling GLSL Shaders
	shader = new Shader(
		"C:/Users/User/source/repos/Multiplayer-Asteroids/CommonClasses/GLSL Shaders/Vertex.txt",
		"C:/Users/User/source/repos/Multiplayer-Asteroids/CommonClasses/GLSL Shaders/Fragment.txt"
	);

	// -- Loading Models -- //
	Actor::loadModelCache(); // Initializes model cache so blueprinted actors can be created

	return 0;
}


void Render()
{
	// ---- Rendering Actors ---- //
	glClearColor(0.1f, 0.1f, 0.1f, 1.f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	shader->use();

	glm::mat4 model = glm::mat4(1.f);
	glm::mat4 view = glm::mat4(1.f);
	glm::mat4 projection = glm::mat4(1.f);

	model = glm::translate(model, glm::vec3(0.f));
	shader->setMat4("model", model);
	view = camera.GetViewMatrix();
	shader->setMat4("view", view);
	projection = glm::perspective(glm::radians(camera.zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.f);
	shader->setMat4("projection", projection);
	for (auto iter = actorMap.begin(); iter != actorMap.end(); ++iter)
	{
		iter->second->Draw(*shader); // Actor handles setting the position offset for shader
	}
	for (auto iter = unlinkedProxies.begin(); iter != unlinkedProxies.end(); iter++)
	{
		iter->second->Draw(*shader);
	}
}


/* This function returns values that are only ever used when the function is called during
 * the CS_PlayingGame state. I've left it this way to cut corners. It works well enough for now 
 */
char processInput(GLFWwindow* window)
{
	char inputBitString = 0b00000000;

	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
	{
		currentState = CS_Exit;
	}

	/*
	* This approach to controlling input based on the state of the program could be done more cleanly
	* by using function pointers, but the time to implement this is needlessly long for the result
	* 
	* NOTE: Small number of cases with Compiler optimization means I'm ok using if statements here
	*/
	if (currentState == CS_WaitingForGameStart)
	{
		if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && !bReadyButtonPressed && bPlayerActorIDSet)
		{
			printf("processInput / CS_WaitingForGameStart -- MSG_STRGM\n");
			bReadyButtonPressed = true;
			sendBuffer[0] = MSG_STRTGM;
			sock.sendData(sendBuffer, 1, serverAddr);
		}
		else if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_RELEASE)
		{
			bReadyButtonPressed = false;
		}
	}
	else if (currentState == CS_PlayingGame)
	{
		// Only process Movement & gameplay action input if controlling character
		if (playerActor == nullptr)
		{
			return inputBitString;
		}

		// -- Movement -- //
		Vector3D moveDirection(0.f);
		if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		{
			moveDirection += Vector3D(0.f, 1.f, 0.f);
			inputBitString |= INPUT_UP;
		}
		if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		{
			moveDirection += Vector3D(0.f, -1.f, 0.f);
			inputBitString |= INPUT_DOWN;
		}
		if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		{
			moveDirection += Vector3D(1.f, 0.f, 0.f);
			inputBitString |= INPUT_RIGHT;
		}
		if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		{
			moveDirection += Vector3D(-1.f, 0.f, 0.f);
			inputBitString |= INPUT_LEFT;
		}

		/* TODO: Eventually remove. These are left here for use with testing later. */
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
		if (glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS && !bFireButtonPressed)
		{
#if PROXIES
			// TESTING
			bShouldPrintTime = true;
			//~
#endif
			spawnProjectile();
			inputBitString |= INPUT_SHOOT;
			bFireButtonPressed = true;
		}
		else if (glfwGetKey(window, GLFW_KEY_J) == GLFW_RELEASE)
		{
			bFireButtonPressed = false;
		}

		// TODO: for testing. Remove later 
		if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS && !bTestButtonPressed)
		{
			bTestButtonPressed = true;

			
			std::cout << "processInpput / test key pressed" << std::endl;

			//std::cout << "handleInput / T" << std::endl;
			//bTestButtonPressed = true;

			//// -- Constructing RPC -- //
			//RemoteProcedureCall rpc;
			//rpc.method = RPC_SPAWN;
			//rpc.secondsSinceLastUpdate = static_cast<float>(glfwGetTime()) - lastFrame;
			//rpc.simulationStep = stateSequenceID; // Works if We're ahead
			//memcpy(&rpc.message, "Hello World", 12);

			//// -- Sending Message -- //
			//memcpy(sendBuffer, &rpc, sizeof(RemoteProcedureCall));
			//sock.sendData(sendBuffer, sizeof(RemoteProcedureCall), serverAddr);
		}
		else if (glfwGetKey(window, GLFW_KEY_T) == GLFW_RELEASE)
		{
			bTestButtonPressed = false;
		}

		
	}//~ playing game loop

	//if (inputBitString != 0b0)
	//{
	//	//inputBitString = inputBitString;
	//	std::cout << "processInput -- inputBitString as Hex : " << std::hex << (int)inputBitString << std::endl;
	//}

	return inputBitString;
}


void spawnProjectile()
{
	if (!playerActor) return;

	//printf("Client::spawnProjectile\n");

	/* -- NOTE -- 
	I didn't feel like this single case for spawning a proxy deserved it's own constructor, as this would
	present a more confusing header for actor. Thus, I'm setting things manually here, since this process should
	only ever be done once */


#if PROXIES
	// TESTING
	spawnTimeStart = static_cast<float>(glfwGetTime());
	//~

	// ---- Constructing Unlinked Proxy ---- //
	Actor* projectile = new Actor(
		++nextProxyID,
		playerActor->getPosition() + (playerActor->getRotation() * 2.f), // Offset is less so true actor and proxy are at approximately the same position on the client 2.f
		playerActor->getRotation(),
		ABI_Projectile,
		true
	);
	projectile->setMoveDirection(Vector3D(1.f, 0.f, 0.f));
	unlinkedProxies.insert({ nextProxyID, projectile });
#endif


	// ---- Sending data to server ---- //
	RemoteProcedureCall rpc;
	rpc.method = RPC_SPAWN;
	rpc.secondsSinceLastUpdate = static_cast<float>(glfwGetTime()) - lastFrame;
	rpc.simulationStep = stateSequenceID; // Works if We're ahead
	memcpy(sendBuffer, &rpc, sizeof(RemoteProcedureCall));
	sock.sendData(sendBuffer, sizeof(RemoteProcedureCall), serverAddr);
}


void destroyActor(unsigned int id)
{
	std::cout << "destroy actor\n";


	Actor* actor = actorMap[id];
	actorMap.erase(id);
	if (playerActor && id == playerActor->getId())
	{
		playerActor = nullptr;
	}
	delete actor;
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
	// TODO: Extending this thought; proxies will likely use a lot of similar code. Having them be treated as actors normally avoids code reuse

	// ---- Moving Unlinked Proxies ---- //
	for (auto iter = unlinkedProxies.begin(); iter != unlinkedProxies.end(); iter++)
	{
		iter->second->addToPosition(iter->second->getMoveDirection() * iter->second->getMoveSpeed() * deltaTime);
	}
}


/*
Reads NUM_PACKETS_PER_CYCLE messages from the receive buffer.
Anything that isnt MSG_REP, will be processed immediately.
Only the most recently received MSG_REP, within NUM_PACKETS_PER_CYCLE, will be processed.
*/
void readRecvBuffer()
{
	// std::cout << "readReceiveBuffer" << std::endl;
	
	// ----  Receiving Data from Server ----  // Receives data from server regardless of state.
	char* tempBuffer{};
	int recvBufferLen = 0;
	int tempBufferLen = 0;
	sockaddr_in recvAddr; // This is never used on purpose. Code smell
	for (int i = 0; i < NUM_PACKETS_PER_CYCLE; i++)
	{
		tempBuffer = sock.recvData(tempBufferLen, recvAddr);

		/* NOTE: For some reason, this block of code will a bug in replication.
		*  If this block is implemented, the client won't create any new actors, suggesting
		*  this block of code permits the discarding of messages we don't want ignored
		*/
		//if (tempBufferLen == -1)
		//{
		//	/*
		//	* Returns -1 because we've set the socket to non-blocking.
		//	* The error returned will be
		//	* WSAEWOULDBLOCK, which is an error code reserved for notifying that things are working as they should be
		//	*/
		//	return;
		//}
		

		// Not a replication message --> Handle right away
		if (*tempBuffer != MSG_REP)
		{
			// std::cout << "!MSG_REP" << std::endl;
			// std::cout << *tempBuffer << " / " << tempBufferLen << std::endl;
			handleMessage(tempBuffer, tempBufferLen);
			continue;
		}

		// Update the values for the most recently seen replication data which is stored in the recvBuffer
		recvBuffer = tempBuffer;
		recvBufferLen = tempBufferLen;
	}


	// Handle the most recently seen replication message within the most recently read NUM_PACKETS_PER_CYCLE messages
	if (recvBufferLen > 0)
	{
		// std::cout << "readRecvBuffer / " << (tempBufferLen / sizeof(ActorNetData)) << std::endl;
		// std::cout << *tempBuffer << " / " << tempBufferLen << std::endl;
		handleMessage(recvBuffer, recvBufferLen);
	}
}


char handleMessage(char* buffer, unsigned int bufferLen)
{
	// std::cout << "handleMessage / bufferLen: " << bufferLen << std::endl;
	// char handledMessage;
	switch (buffer[0])
	{
	case MSG_CONNECT:	// Connection Reply
	{
		/* -- NOTE --
		* Ideally, controlledActorID will always be ID 0 from this clients pool of network IDs.
		* I haven't bothered to logically confirm that this will always happen
		* so instead I set the next predicted network ID to the received ID + 1, just to be safe
		*/
		printf("handleMessage / MSG_CONNECT\n");
		ConnectAckData data;
		memcpy(&data, recvBuffer, sizeof(ConnectAckData));
		controlledActorID = data.controlledActorID;
		bPlayerActorIDSet = true;
		printf("    > controlledActorID: %u\n", (!CLIENT_NETWORK_ID_MASK) & controlledActorID);
		nextPredictedOwnedActorNetworkID = controlledActorID + 1;
		clientNetworkID = data.clientNetworkID;
		printf("    > clientNetworkID: %d\n", clientNetworkID);
		currentState = CS_WaitingForGameStart;
		return MSG_CONNECT;
	}
	case MSG_STRTGM:
	{
		printf("handleMessage / MSG_STRTGM\n");
		StartGameData data;
		memcpy(&data, buffer, sizeof(StartGameData));
		stateSequenceID = 5 + data.simulationStep; // 5 + ...
		printf("   Received State Sequence ID : %d\n", stateSequenceID);
		currentState = CS_PlayingGame;
		return MSG_STRTGM;
	}
	case MSG_TSTEP:	// Receive current simulation step
	{
		memcpy(&stateSequenceID, buffer + 1, sizeof(unsigned int));
		stateSequenceID += 5; // 5
		printf("handleMessage / MSG_TSTEP -- Received State Sequence ID : %d\n", stateSequenceID);
		return MSG_TSTEP;
	}
	case MSG_REP:	// Replicating
	{
		// std::cout << "handleMessage / MSG_REP" << std::endl; 
		
		/* Updates actor states to match server data.
		*  Spawns actors that were sent to client, but don't yet exist */
		//replicateState(++buffer, --bufferLen); // TODO: Fixed update actors shouldn't be here. It should always be called and check for missing states when comparing
		
		ServerGameState state;
		memcpy(&state, recvBuffer, sizeof(ServerGameState));

		replicateState(state);

		return MSG_REP;
	}
	case MSG_ID:	// Receiving Controlled Actor ID
	{
		memcpy(&controlledActorID, ++buffer, sizeof(unsigned int));
		bPlayerActorIDSet = true;
		printf("handleMessage / MSG_ID : controlledActorID: %d\n", controlledActorID);
		return MSG_ID;
	}
//	case MSG_SPAWN:
//	{
//		printf("handleMessage / MSG_SPAWN\n");
//
//		/* Handling server Reply for Spawn Request
//		*  Links locally spawned dummy with authoritative server copy
//		*/
//		NetworkSpawnData data;
//		memcpy(&data, buffer, sizeof(NetworkSpawnData));
//#if PROXIES
//		/*
//		unlinked proxy becomes a normal actor. This avoids the overhead cost of constructing a new actor when required
//		*/
//		Actor* linkedActor = unlinkedProxies[data.dummyActorID];
//		linkedActor->setId(data.networkedActorID);
//		actorMap[data.networkedActorID] = linkedActor;
//		unlinkedProxies.erase(data.dummyActorID);
//
//		printf("  (dummyActorID, networkedActorID) : (%d , %d)\n", data.dummyActorID, data.networkedActorID);
//		return MSG_SPAWN;
//#endif
//	}
	}
}




/* DEPRECATED. Use other version
* Sets the state of each actor found in the buffer.
* Local actors not found in the buffer were destroyed on the server. This is replicated here
* Actors that don't exist locally must have been created and are thus, created here
* 
* Creates the state object used for Client Reconciliation.
*/
void replicateState(char* buffer, int bufferLen)
{
	// std::cout << "Client::replicateState\n";
	// printf("replicateState / actors.size() == %d\n", actorMap.size());
	// std::cout << "replicateState / num actors in buffer: " << bufferLen / sizeof(ActorNetData) << std::endl;

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


	// std::cout << "replicateState / numActorsReceived: " << numActorsReceived << std::endl;


	// -- Creating and Copying to stateSequenceID -- //
	unsigned int receivedStateSequenceID = -1;
	memcpy(&receivedStateSequenceID, buffer, sizeof(unsigned int));
	buffer += sizeof(unsigned int);
	if (stateSequenceID == 0)
	{
		printf("	Received State: %d\n	Set State: %d\n", receivedStateSequenceID, stateSequenceID);
		stateSequenceID = receivedStateSequenceID + 5; 
	}


	// -- Reading Actors -- //
	Actor* actor = nullptr;
	for (unsigned int i = 0; i < numActorsReceived; i++)
	{
		ActorNetData data;
		memcpy(&data, buffer, sizeof(data));


		
#if PROXIES
		// TESTING
		if (data.id != controlledActorID && bShouldPrintTime)
		{
			float spawnLatencyReturnTime = static_cast<float>(glfwGetTime());
			spawnLatencySeconds = spawnLatencyReturnTime - spawnTimeStart;
			std::cout << "TESTING - replicateState / spawnLatencySeconds: " << spawnLatencySeconds << std::endl;
			bShouldPrintTime = false;
		}
		//~
#endif


		// -- Observerving type of data received -- //
		if (data.bIsDestroyed) // Actor has been destroyed on the server 
		{
			destroyActor(data.id);
		}
		else if (actorMap.count(data.id) == 0) // New Actor Encountered
		{
			// -- Add actor to map -- //
			actor = new Actor(data.id, data.Position, data.Rotation, data.blueprintID, true);
			actorMap[data.id] = actor;

			// Is there a corresponding proxy that can be deleted?
			if (unlinkedProxies.count(data.id) != 0)
			{
				// delete proxy
				// delete unlinkedProxies[data.id];
				// unlinkedProxies.erase(unlinkedProxies.find(data.id));
			}
		}
		else // -- Actor found. Updating Actor data -- //
		{
			actor = actorMap[data.id];
			actor->setPosition(data.Position);
			actor->setRotation(data.Rotation);
			actor->setMoveDirection(data.moveDirection);
			actor->setMoveSpeed(data.moveSpeed);
		}

		// -- Checking to see if we've encountered the player's controlled actor -- //
		if (data.id == controlledActorID && playerActor == nullptr)
		{
			printf("Client::replicateState -- Updating playerActor\n");
			playerActor = actor;
		}
		// -- Incrementing Buffer Pointer -- 
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
}





void replicateState(ServerGameState& state)
{
	lastServerGameState = state;

	if (storedInputRequests.size() == 0) return;

	//-----------------------------------------------------//
	//---- Popping older states than the one just ACKd ----//
	//-----------------------------------------------------//
	std::vector<ClientInputRequest*>::iterator reqItr = storedInputRequests.begin();
	while (reqItr != storedInputRequests.end())
	{
		// If most recently acknowledge state, break
		if ((*reqItr)->inputRequestID == state.acknowledgedRequestID)
		{	
			delete (*reqItr);
			storedInputRequests.erase(reqItr);
			break;
		}

		// delete state
		delete (*reqItr);
		reqItr = storedInputRequests.erase(reqItr);
	} 
	storedInputRequests.shrink_to_fit();


	//-------------------------------------------------------//
	//---- Setting State of world to that of Acked State ----//
	//-------------------------------------------------------//
	ActorNetData* netData = state.actorNetData;
	Actor* actor = nullptr;
	for (int i = 0; i < state.numActors; i++)
	{
		//std::cout << "  " << netData->id << std::endl;

		//-- Observerving type of data received
		if (netData->bIsDestroyed) // Actor has been destroyed on the server 
		{
			destroyActor(netData->id);
		}
		else if (actorMap.find(netData->id) == actorMap.end()) // New Actor Encountered. Adding Actor to map
		{
			actor = new Actor(netData->id, netData->Position, netData->Rotation, netData->blueprintID, true);
			actorMap[netData->id] = actor;
		}
		else //-- Actor found. Updating Actor data
		{
			auto actorItr = actorMap.find(netData->id);
			actorItr->second->setPosition(netData->Position);
			actorItr->second->setRotation(netData->Rotation);
			actorItr->second->setMoveDirection(netData->moveDirection);
			actorItr->second->setMoveSpeed(netData->moveSpeed);
		}

		// -- Checking to see if we've encountered the player's controlled actor 
		if (netData->id == controlledActorID && playerActor == nullptr)
		{
			printf("Client::replicateState -- Updating playerActor\n");
			playerActor = actor;
		}
		netData++;
	}//~ Setting State of World


	//-----------------------------------------------//
	//---- Reappling all non-ACKd stored changes ----//
	//-----------------------------------------------//
	if (!playerActor) return;
	for (ClientInputRequest* req : storedInputRequests)
	{
		playerActor->addToPosition(playerActor->getMoveDirection() * playerActor->getMoveSpeed() * storedInputRequests.front()->deltaTime);
	}
}





void clientReconciliation()
{
	// TODO
}






void cleanup()
{
	printf("cleanup");
	if (bWinsockLoaded) WSACleanup();
	glfwTerminate();
}


