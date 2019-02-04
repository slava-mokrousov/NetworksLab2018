#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <iostream>

// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

void showMenu();
int readSocket(SOCKET server, char* buffer, int bufferSize);
void sendToSocket(SOCKET server, const char* buffer);

SOCKET serverSocket = INVALID_SOCKET;

int main(int argc, char *argv[]) {
	//if (argc < 3) {
	//	fprintf(stderr, "usage %s hostname port\n", argv[0]);
	//	exit(0);
	//}
	WSADATA wsaData;
	struct addrinfo *result = NULL,
		*ptr = NULL,
		hints;
	int iResult;

	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed with error: %d\n", iResult);
		return 1;
	}

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	// Resolve the server address and port
	iResult = getaddrinfo("127.0.0.1", "5001", &hints, &result);
	if (iResult != 0) {
		printf("getaddrinfo failed with error: %d\n", iResult);
		WSACleanup();
		return 1;
	}

	// Attempt to connect to an address until one succeeds
	for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {

		// Create a SOCKET for connecting to server
		serverSocket = socket(ptr->ai_family, ptr->ai_socktype,
			ptr->ai_protocol);
		if (serverSocket == INVALID_SOCKET) {
			printf("socket failed with error: %ld\n", WSAGetLastError());
			WSACleanup();
			return 1;
		}

		// Connect to server.
		iResult = connect(serverSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
		if (iResult == SOCKET_ERROR) {
			closesocket(serverSocket);
			serverSocket = INVALID_SOCKET;
			continue;
		}
		break;
	}

	freeaddrinfo(result);

	if (serverSocket == INVALID_SOCKET) {
		printf("Unable to connect to server!\n");
		WSACleanup();
		return 1;
	}

	showMenu();
	closesocket(serverSocket);
}

void showMenu() {
	char buffer[1024];
	while (TRUE) {
		memset(buffer, 0, sizeof(buffer));
		printf("1 to get polls\r\n");
		printf("2 to get poll by theme\r\n");
		printf("3 to create poll\r\n");
		printf("4 to delete poll\r\n");
		printf("5 to add option to poll\r\n");
		printf("6 to close poll\r\n");
		printf("7 to add votes\r\n");
		std::string action;
		std::cin >> action;
		if (action.compare("1") == 0) {
			sendToSocket(serverSocket, "getpolls\n");
		}
		else if (action.compare("2") == 0) {
			std::string theme;
			std::cin >> theme;
			sendToSocket(serverSocket, (std::string("getpoll\n") + theme + "\n").c_str());
		}
		else if (action.compare("3") == 0) {
			std::string theme;
			std::cin >> theme;
			std::string options;
			std::string option;
			while (TRUE) {
				std::cin >> option;
				if (option.compare(".") == 0) {
					break;
				}
				options += option + "\n";
			}
			sendToSocket(serverSocket, (std::string("createpoll\n") + theme + "\n" + options).c_str());
		}
		else if (action.compare("4") == 0) {
			std::string theme;
			std::cin >> theme;
			sendToSocket(serverSocket, (std::string("deletepoll\n") + theme + "\n").c_str());
		}
		else if (action.compare("5") == 0) {
			std::string theme;
			std::cin >> theme;
			std::string options;
			std::string option;
			while (TRUE) {
				std::cin >> option;
				if (option.compare(".") == 0) {
					break;
				}
				options += option + "\n";
			}
			sendToSocket(serverSocket, (std::string("addoptions\n") + theme + "\n" + options).c_str());
		}
		else if (action.compare("6") == 0) {
			std::string theme;
			std::cin >> theme;
			sendToSocket(serverSocket, (std::string("closepoll\n") + theme + "\n").c_str());
		}
		else if (action.compare("7") == 0) {
			std::string theme;
			std::cin >> theme;
			std::string option;
			std::cin >> option;
			std::string count;
			std::cin >> count;
			sendToSocket(serverSocket, (std::string("addvotes\n") + theme + "\n" + option + "\n" + count + "\n").c_str());
		}
		else {
			continue;
		}
		char answer[1024];
		int readCount = readSocket(serverSocket, answer, sizeof(answer));
		if (readCount <= 0) {
			return;
		}
		printf(answer);
		printf("\r\n");
	}
}

int readSocket(SOCKET server, char* buffer, int bufferSize) {
	memset(buffer, 0, bufferSize);
	int n = recv(server, buffer, bufferSize - 1, 0);
	return n;
}

void sendToSocket(SOCKET server, const char* buffer) {
	send(server, buffer, strlen(buffer), 0);
}