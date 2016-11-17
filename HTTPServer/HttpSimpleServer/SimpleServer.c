// BasicTcpServer.cpp : Defines the entry point for the console application.
//
#undef UNICODE
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>

#include "HTTPRequest.h"
#include "HTTPResponse.h"
#include "HTTPCommons.h"

// FIXME
#include <time.h>

// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")

#define MAX_BUFF_LEN 16384
#define DEFAULT_PORT "8888"
#define SLEEP_TIME 1000

void acquireHttpRequest(HttpRequest *httpRequest, SOCKET ListenSocket, SOCKET ClientSocket);
int main(void) {
    WSADATA wsaData;
    int iResult, iSendResult;

    SOCKET ListenSocket = INVALID_SOCKET;
    SOCKET ClientSocket = INVALID_SOCKET;

	// Addresses
    struct addrinfo *result = NULL;
    struct addrinfo hints;

	// Http Request and response data
	HttpRequest httpRequest;

	u_long iMode;
    
    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed with error: %d\n", iResult);
        return EXIT_ERROR;
    }

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    // Resolve the server address and port
    iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
    if ( iResult != 0 ) {
        printf("getaddrinfo failed with error: %d\n", iResult);
        WSACleanup();
        return EXIT_ERROR;
    }

    // Create a SOCKET for connecting to server
    ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (ListenSocket == INVALID_SOCKET) {
        printf("socket failed with error: %ld\n", WSAGetLastError());
        freeaddrinfo(result);
        WSACleanup();
        return EXIT_ERROR;
    }

    // Setup the TCP listening socket
    iResult = bind( ListenSocket, result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        printf("bind failed with error: %d\n", WSAGetLastError());
        freeaddrinfo(result);
        closesocket(ListenSocket);
        WSACleanup();
        return EXIT_ERROR;
    }

    freeaddrinfo(result);

    iResult = listen(ListenSocket, SOMAXCONN);
    if (iResult == SOCKET_ERROR) {
        printf("listen failed with error: %d\n", WSAGetLastError());
        closesocket(ListenSocket);
        WSACleanup();
        return EXIT_ERROR;
    }

	// Keep the server running.
	while(1) {
		// Accept a client socket
		ClientSocket = accept(ListenSocket, NULL, NULL);
		if (ClientSocket == INVALID_SOCKET) {
			printf("accept failed with error: %d\n", WSAGetLastError());
			closesocket(ListenSocket);
			//continue;
		}
		// Nonblocking socket
		iMode=1;
		ioctlsocket(ClientSocket,FIONBIO,&iMode);

		acquireHttpRequest(&httpRequest, ListenSocket, ClientSocket);
		
		sendHttpResponse(httpRequest, ClientSocket);
		
		printf("Connection closing...\n");
		iResult = shutdown(ClientSocket, SD_SEND);
		if (iResult == SOCKET_ERROR) {
			printf("shutdown failed with error: %d\n", WSAGetLastError());
		}
		closesocket(ClientSocket);

		freeHttpRequest(&httpRequest);
	}
    
    // shutdown the connection since we're done
    iResult = shutdown(ListenSocket, SD_SEND);
    if (iResult == SOCKET_ERROR) {
        printf("shutdown failed with error: %d\n", WSAGetLastError());
        closesocket(ListenSocket);
        WSACleanup();
        return EXIT_ERROR;
    }
	// No longer need server socket
    closesocket(ListenSocket);
    WSACleanup();

    return EXIT_OK;
}

void acquireHttpRequest(HttpRequest *httpRequest, SOCKET ListenSocket, SOCKET ClientSocket) {
	int iSendResult, iResult, msgLen, errorCode;
	char recvbuf[DEFAULT_BUFLEN], msgbuf[MAX_BUFF_LEN];
	int recvbuflen = DEFAULT_BUFLEN, msgbuflen = 0;
	BOOL dataRecieved = FALSE;
	// Initialize the structure with NULL values.
	httpRequest->requestLine = NULL;
	httpRequest->headers = NULL;
	httpRequest->entityBody = NULL;
	
	iResult = iSendResult = -1;
	
	do {
		Sleep(SLEEP_TIME);
		ZeroMemory(recvbuf, recvbuflen);
		msgLen = recv(ClientSocket, recvbuf, recvbuflen, 0);
		if(msgLen == SOCKET_ERROR) {
			errorCode = WSAGetLastError();
			if(errorCode != WSAEWOULDBLOCK) {
				printf("recv failed with error: %d\n", errorCode);
				break;
			}
		} else {
			memcpy(msgbuf + msgbuflen, recvbuf, msgLen);
			msgbuflen += msgLen;
		}
	} while(msgLen > 0);
	
	msgbuf[msgbuflen] = '\0';
	*httpRequest = parseHttpMessage(msgbuf, msgbuflen);
	printf("Request:\n%s\n", msgbuf);
	printf("Bytes received: %d\n", msgbuflen);
}