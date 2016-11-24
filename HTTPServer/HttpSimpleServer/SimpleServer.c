// BasicTcpServer.cpp : Defines the entry point for the console application.
#undef UNICODE
#include "SimpleServer.h"

int main(void) {
    WSADATA wsaData;
    int iResult;

    SOCKET ListenSocket = INVALID_SOCKET;
    SOCKET ClientSocket = INVALID_SOCKET;
	HttpRequest *httpRequest = NULL;

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
	
	// Listen for requests.
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
			WSACleanup();
			return EXIT_ERROR;
		}

		if(acquireHttpRequest(ListenSocket, ClientSocket, &httpRequest) == EXIT_OK) {
			if(httpRequest != NULL) {
				sendHttpResponse(httpRequest, &ClientSocket);
				freeHttpRequest(httpRequest);
				free(httpRequest);
			}			
		}

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

/*
This function is responsible for processing the HTTP request sent by clients and storing
the request in the structure HttpRequest.

INPUT: 'ListenSocket' is the server socket. 'ClientSocket' is the client socket. 'httpRequest'
stores the processed HTTP request.

OUTPUT: The exit code of the operation.
*/
int acquireHttpRequest(SOCKET ListenSocket,
					   SOCKET ClientSocket,
					   HttpRequest** httpRequest) {
	int iSendResult, iResult, msgLen, errorCode;
	char recvbuf[DEFAULT_BUFLEN], msgbuf[MAX_BUFF_LEN];
	int recvbuflen = DEFAULT_BUFLEN, msgbuflen = 0;
	BOOL dataRecieved = FALSE;
	u_long availableBytes;
	fd_set set;
	struct timeval tm;
	
	tm.tv_sec = WAIT_TIME_SEC;
	tm.tv_usec = 0;

	// The request structure is nullified in case no message is received.
	*httpRequest = NULL;
	iSendResult = -1;

	/*
	The recv function is blocking - if the underlying buffer is empty,
	the function will block awaitng new data. To make sure that the program
	doesn't stall at that point, a check is performed to verify that no data
	remains unprocessed inside the buffer. If the buffer is empty, then the
	acquired data (the request message) is parsed.
	*/
	FD_ZERO(&set);
	FD_SET(ClientSocket, &set);
	memset(&msgbuf, 0, MAX_BUFF_LEN);
	msgbuflen = 0;
	do {
		// Check if the client socket has data pending for processing.

		select(0, &set, NULL, NULL, &tm);
		// FIXME - catch error
		// If no data remains in the underlying buffer, move onto parsing the message.
		if (ioctlsocket(ClientSocket,FIONREAD,&availableBytes) < 0) break;
		printf("\npending = %ld\n", availableBytes);
		if (availableBytes == 0) {
			printf("nothing to do\n");
			break;
		}

		/*
		If there is data inside the underlying buffer, copy it into the local message buffer,
		which will store the entire request.
		The request is copied block by block inside the local buffer. 'recvbuf' will store
		each block and then the block will is appeneded at the end of the local message buffer.
		*/
		ZeroMemory(&recvbuf, sizeof(DEFAULT_BUFLEN));
		msgLen = recv(ClientSocket, recvbuf, recvbuflen, 0);
		if (msgLen > 0) {
			// Append at the end of the local message buffer.
			recvbuf[msgLen] = '\0';				
			memcpy(msgbuf + msgbuflen, recvbuf, msgLen);
			msgbuflen += msgLen;
		} else if(msgLen == SOCKET_ERROR) {
			// If an error occured, acquired it and return immediately with the erro code.
			errorCode = WSAGetLastError();				
			printf("recv failed with error: %d\n", errorCode);
			return errorCode;
		}
	} while(1);
	
	// Create the request structure if a message was received.
	if (msgbuflen > 0) {
		// Initialize the structure with NULL values.
		msgbuf[msgbuflen] = '\0';
		(*httpRequest) = parseHttpMessage(msgbuf, msgbuflen);
		printf("Request:\n%s\n", msgbuf);
		printf("Bytes received: %d\n", msgbuflen);
	}

	return EXIT_OK;
}
