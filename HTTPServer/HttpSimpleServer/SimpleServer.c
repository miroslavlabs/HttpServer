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

// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")

#define MAX_BUFF_LEN 16384
#define DEFAULT_PORT "8888"
#define SLEEP_TIME 1000

void acquireHttpRequest(SOCKET ListenSocket, SOCKET ClientSocket);

int main(void) {
    WSADATA wsaData;
    int iResult;
	int errorCode;
	u_long availableBytes;
	fd_set set;
	struct timeval tm;

    SOCKET ListenSocket = INVALID_SOCKET;
    SOCKET ClientSocket = INVALID_SOCKET;

	// Addresses
    struct addrinfo *result = NULL;
    struct addrinfo hints;

    int iSendResult;
    char recvbuf[DEFAULT_BUFLEN];
    int recvbuflen = DEFAULT_BUFLEN;
    
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

	tm.tv_sec = 1;
	tm.tv_usec = 0;
	// Keep the server running.
	while(1) {
		// Accept a client socket
		ClientSocket = accept(ListenSocket, NULL, NULL);
		if (ClientSocket == INVALID_SOCKET) {
			printf("accept failed with error: %d\n", WSAGetLastError());
			closesocket(ListenSocket);
			WSACleanup();
			return EXIT_ERROR;
		}// Receive until the peer shuts down the connection

		acquireHttpRequest(ListenSocket, ClientSocket);

		printf("Connection closing...\n");
		iResult = shutdown(ClientSocket, SD_SEND);
		if (iResult == SOCKET_ERROR) {
			printf("shutdown failed with error: %d\n", WSAGetLastError());
		}
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

void acquireHttpRequest(SOCKET ListenSocket, SOCKET ClientSocket) {
	int iSendResult, iResult, msgLen, errorCode;
	char recvbuf[DEFAULT_BUFLEN], msgbuf[MAX_BUFF_LEN];
	int recvbuflen = DEFAULT_BUFLEN, msgbuflen = 0;
	BOOL dataRecieved = FALSE;
	HttpRequest *httpRequest;

	u_long availableBytes;
	fd_set set;
	struct timeval tm;
	tm.tv_sec = 2;
	tm.tv_usec = 0;

	// Initialize the structure with NULL values.
	httpRequest = (HttpRequest*)malloc(sizeof(HttpRequest));
	httpRequest->requestLine = NULL;
	httpRequest->headers = NULL;
	httpRequest->entityBody = NULL;
	
	iSendResult = -1;

	FD_ZERO(&set);
	FD_SET(ClientSocket, &set);
	memset(&msgbuf, 0, MAX_BUFF_LEN);
	msgbuflen = 0;
	do {
		select(0, &set, NULL, NULL, &tm);
		// FIXME - catch error
		if (ioctlsocket(ClientSocket,FIONREAD,&availableBytes) < 0) break;
		printf("\npending = %ld\n", availableBytes);
		if (availableBytes == 0) {
			break;
		}
		ZeroMemory(&recvbuf, sizeof(DEFAULT_BUFLEN));
		msgLen = recv(ClientSocket, recvbuf, recvbuflen, 0);
		if (msgLen > 0) {
			recvbuf[msgLen] = '\0';				
			memcpy(msgbuf + msgbuflen, recvbuf, msgLen);
			msgbuflen += msgLen;
		} else if(msgLen == SOCKET_ERROR) {
			errorCode = WSAGetLastError();				
			printf("recv failed with error: %d\n", errorCode);
			closesocket(ClientSocket);
			WSACleanup();
			break;
		}
	} while(1);
	
	if (msgbuflen > 0) {
		msgbuf[msgbuflen] = '\0';
		*httpRequest = parseHttpMessage(msgbuf, msgbuflen);
		printf("Request:\n%s\n", msgbuf);
		printf("Bytes received: %d\n", msgbuflen);

		sendHttpResponse(httpRequest, &ClientSocket);
		freeHttpRequest(httpRequest);
		free(httpRequest);
	}
}