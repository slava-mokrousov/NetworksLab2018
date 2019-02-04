#include "Poll.h"
#include <vector>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#pragma comment (lib, "Ws2_32.lib")

#define DEFAULT_PORT "5001"

DWORD WINAPI connectionWorker(LPVOID lpParam);
DWORD WINAPI clientWorker(LPVOID lpParam);
BOOL WINAPI closeServer(DWORD fdwCtrlType);
void closeServer();
void closeClient(SOCKET clientSocket);
int readSocket(SOCKET clientSocket, char* buffer, int bufferSize);
void sendToSocket(SOCKET clientSocket, const char* buffer);
void parseParameters(char* buffer, std::vector<std::string>& parameters);
void sendBadRequest(SOCKET clientSocket);

std::vector<Poll> polls;

std::vector<SOCKET> clients;

SOCKET serverSocket = INVALID_SOCKET;

int main() {
	SetConsoleCtrlHandler(closeServer, TRUE);

	WSADATA wsaData;
	int iResult;

	struct addrinfo *result = NULL;
	struct addrinfo hints;

	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed with error: %d\n", iResult);
		return 1;
	}

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	// Resolve the server address and port
	iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
	if (iResult != 0) {
		printf("getaddrinfo failed with error: %d\n", iResult);
		WSACleanup();
		return 1;
	}

	// Create a SOCKET for connecting to server
	serverSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (serverSocket == INVALID_SOCKET) {
		printf("socket failed with error: %ld\n", WSAGetLastError());
		freeaddrinfo(result);
		WSACleanup();
		return 1;
	}

	// Setup the TCP listening socket
	iResult = bind(serverSocket, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		printf("bind failed with error: %d\n", WSAGetLastError());
		freeaddrinfo(result);
		closesocket(serverSocket);
		WSACleanup();
		return 1;
	}

	freeaddrinfo(result);

	iResult = listen(serverSocket, SOMAXCONN);
	if (iResult == SOCKET_ERROR) {
		printf("listen failed with error: %d\n", WSAGetLastError());
		closesocket(serverSocket);
		WSACleanup();
		return 1;
	}

	if (CreateThread(NULL, 0, connectionWorker, NULL, 0, NULL) == NULL) {
		printf("can't start connection worker\r\n");
	}

	while (TRUE) {
	}
}

DWORD WINAPI connectionWorker(LPVOID lpParam) {
	SOCKET clientSocket;
	while (TRUE) {
		clientSocket = accept(serverSocket, NULL, NULL);
		if (clientSocket == INVALID_SOCKET) {
			printf("accept failed with error: %d\n", WSAGetLastError());
			closesocket(clientSocket);
			continue;
		}
		if (CreateThread(NULL, 0, clientWorker, &clientSocket, 0, NULL) == NULL) {
			printf("can't start connection worker\r\n");
			continue;
		}
		clients.push_back(clientSocket);
	}
}

DWORD WINAPI clientWorker(LPVOID lpParam) {
	SOCKET clientSocket = *(SOCKET*)lpParam;
	char buffer[1024];
	while (TRUE) {
		int count = readSocket(clientSocket, buffer, sizeof(buffer));
		if (count <= 0) {
			break;
		}
		std::vector<std::string> parameters;
		parseParameters(buffer, parameters);
		if (parameters.size() < 1) {
			sendBadRequest(clientSocket);
			continue;
		}
		std::string method = parameters[0];
		std::string response;
		if (method.compare("getpolls") == 0) {
			response += "1\r\n";
			for (int index = 0; index < polls.size(); ++index) {
				response += polls[index].theme + "\r\n";
			}
		}
		else if (method.compare("getpoll") == 0) {
			if (parameters.size() < 2) {
				sendBadRequest(clientSocket);
				continue;
			}
			std::string theme = parameters[1];
			for (int index = 0; index < polls.size(); ++index) {
				Poll& poll = polls[index];
				if (poll.theme.compare(theme) == 0) {
					response = "1\r\n" + poll.description();
					break;
				}
			}
		}
		else if (method.compare("createpoll") == 0) {
			if (parameters.size() < 3) {
				sendBadRequest(clientSocket);
				continue;
			}
			std::string theme = parameters[1];
			std::vector<std::string> options;
			for (int index = 2; index < parameters.size(); ++index) {
				options.push_back(parameters[index]);
			}
			polls.push_back(Poll(theme, options));
			response = "1\r\nOK";
		}
		else if (method.compare("deletepoll") == 0) {
			if (parameters.size() < 2) {
				sendBadRequest(clientSocket);
				continue;
			}
			std::string theme = parameters[1];
			for (int index = 0; index < polls.size(); ++index) {
				if (polls[index].theme == theme) {
					polls.erase(polls.begin() + index);
					response = "1\r\nOK";
					break;
				}
			}
		}
		else if (method.compare("addoptions") == 0) {
			if (parameters.size() < 3) {
				sendBadRequest(clientSocket);
				continue;
			}
			std::string theme = parameters[1];
			std::vector<std::string> options;
			for (int index = 2; index < parameters.size(); ++index) {
				options.push_back(parameters[index]);
			}
			bool result = true;
			for (int index = 0; index < polls.size(); ++index) {
				Poll& poll = polls[index];
				if (poll.theme == theme) {
					for (int index = 0; index < options.size(); ++index) {
						result |= poll.addOption(options[index]);
					}
					break;
				}
			}
			if (result) {
				response = "1\r\nOK";
			}
		}
		else if (method.compare("closepoll") == 0) {
			if (parameters.size() < 2) {
				sendBadRequest(clientSocket);
				continue;
			}
			std::string theme = parameters[1];
			for (int index = 0; index < polls.size(); ++index) {
				Poll& poll = polls[index];
				if (poll.theme == theme) {
					poll.close();
					response = "1\r\nOK";
					break;
				}
			}
		}
		else if (method.compare("addvotes") == 0) {
			if (parameters.size() < 4) {
				sendBadRequest(clientSocket);
				continue;
			}
			std::string theme = parameters[1];
			std::string option = parameters[2];
			int count = std::stoi(parameters[3]);

			for (int index = 0; index < polls.size(); ++index) {
				Poll& poll = polls[index];
				if (poll.theme == theme) {
					if (poll.addVotes(option, count)) {
						response = "1\r\nOK";
					}
					break;
				}
			}
		}
		else {
			sendBadRequest(clientSocket);
			continue;
		}

		if (response.empty()) {
			sendBadRequest(clientSocket);
			continue;
		}

		sendToSocket(clientSocket, response.c_str());
	}

	closeClient(clientSocket);
	return 0;
}

BOOL WINAPI closeServer(DWORD fdwCtrlType) {
	closeServer();
	return TRUE;
}

void closeServer() {
	closesocket(serverSocket);
	for (int index = 0; index < clients.size(); ++index) {
		closesocket(clients[index]);
	}
	while (clients.size() != 0)
	{
	}
}

void closeClient(SOCKET clientSocket) {
	for (int index = 0; index < clients.size(); ++index) {
		SOCKET& client = clients[index];
		if (client == clientSocket) {
			closesocket(client);
			clients.erase(clients.begin() + index);
			break;
		}
	}
}

int readSocket(SOCKET clientSocket, char* buffer, int bufferSize) {
	memset(buffer, 0, bufferSize);
	int n = recv(clientSocket, buffer, bufferSize - 1, 0);
	return n;
}

void sendToSocket(SOCKET clientSocket, const char* buffer) {
	send(clientSocket, buffer, strlen(buffer), 0);
}

void parseParameters(char* buffer, std::vector<std::string>& parameters) {
	char* prevSplitPtr = buffer;
	char* splitPtr = NULL;
	do {
		char parameter[256];
		splitPtr = strchr(prevSplitPtr, '\n');
		if (splitPtr != NULL) {
			memset(parameter, 0, sizeof(parameter));
			strncpy_s(parameter, prevSplitPtr, splitPtr - prevSplitPtr);
			parameters.push_back(parameter);
		}
		prevSplitPtr = splitPtr + 1;
	} while (splitPtr != NULL);
}

void sendBadRequest(SOCKET clientSocket) {
	char buffer[] = "0\r\nBAD REQUEST";
	sendToSocket(clientSocket, buffer);
}
