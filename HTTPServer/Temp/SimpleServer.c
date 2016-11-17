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

// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")

#define DEFAULT_BUFLEN 1024 // 1KB
#define MSG_BUFLEN 16834 // 16KB
#define DEFAULT_PORT "8888"
#define SOCKET_TIMEOUT_MILLISECONDS 300

#define NO_DATA_RECEIVED 0
#define EXIT_ERROR 1
#define EXIT_OK 0

int main(void) {
    WSADATA wsaData;
    int iResult;
	struct timeval tv;

    SOCKET ListenSocket = INVALID_SOCKET;
    SOCKET ClientSocket = INVALID_SOCKET;

	// Addresses
    struct addrinfo *result = NULL;
    struct addrinfo hints;

	// Http Request and response data
	HttpRequest httpRequest;

    int iSendResult, currMsgBufPos;
	char recvbuf[DEFAULT_BUFLEN], msgbuf[MSG_BUFLEN];
    int recvbuflen = DEFAULT_BUFLEN, msgbuflen = MSG_BUFLEN;
    
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

	currMsgBufPos = 0;
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
		
		// Set the timeout of the socket
		tv.tv_sec = SOCKET_TIMEOUT_MILLISECONDS;
		setsockopt(ClientSocket, SOL_SOCKET, SO_RCVTIMEO,(char*)&tv,sizeof(struct timeval));
		do {
			iResult = recv(ClientSocket, recvbuf, recvbuflen, MSG_WAITALL);
			if (iResult > 0) {
				memcpy(msgbuf + currMsgBufPos, recvbuf, iResult);
				currMsgBufPos += iResult;
				ZeroMemory(recvbuf, DEFAULT_BUFLEN);
			}
		} while(iResult > NO_DATA_RECEIVED);

		msgbuf[currMsgBufPos] = '\0';
		printf("Request:\n%s\n", msgbuf);
		printf("Bytes received: %d\n", currMsgBufPos);

		// Echo the buffer back to the sender
		iSendResult = send( ClientSocket, msgbuf, currMsgBufPos, 0 );
		if (iSendResult == SOCKET_ERROR) {
			printf("send failed with error: %d\n", WSAGetLastError());
			closesocket(ClientSocket);
			WSACleanup();
			return EXIT_ERROR;
		}

		printf("Bytes sent: %d\n", iSendResult);
		printf("Connection closing...\n");
		iResult = shutdown(ClientSocket, SD_SEND);
		if (iResult == SOCKET_ERROR) {
			printf("shutdown failed with error: %d\n", WSAGetLastError());
		}
		closesocket(ClientSocket);

		httpRequest = parseHttpMessage(msgbuf, currMsgBufPos);
		printf("Method: %d\nURI: %s\nHTTP version: %d\n", httpRequest.requestLine->method,
			httpRequest.requestLine->requestURI, httpRequest.requestLine->httpVersion);
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