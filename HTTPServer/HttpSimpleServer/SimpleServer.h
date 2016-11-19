#ifndef SIMPLE_SERVER_H
#define SIMPLE_SERVER_H

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

int acquireHttpRequest(__in SOCKET ListenSocket,
					   __in SOCKET ClientSocket,
					   __out HttpRequest **httpRequest);

#endif