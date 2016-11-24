#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

#include <Windows.h>
#include "HTTPCommons.h"
#include "HTTPRequest.h"
#include <stdio.h>

/*
The structure contains the status of an HTTP response.
*/
typedef struct __status {
	char *statusCode;
	char *reasonPhrase;
} Status;

/*
The strucure contains all of the supported statuses returned
by the HTTP server.
*/
struct __status_codes_table {
	// These bolong to the "Success" class;
	Status OK;
	Status Created;
	Status NoContent;

	// These belong to the "Client Error" class.
	Status BadRequest;
	Status NotFound;

	// These belong to the "Server Error" class.
	Status NotImplemented;
}StatusCodesTable;

/*
The structure contains all of the parts of an HTTP response - 
the HTTP version, status, response header section and response body.
*/
typedef struct __http_response {
	char *httpVersion;
	Status *status;
	HttpHeaders *headers;
	char *body;
} HttpResponse;

void initStatusCodes();

int sendHttpResponse(HttpRequest *httpRequest,
					 SOCKET *ClientSocket);

char *createHttpResponseAsString(HttpResponse *httpResponse,
							   HttpRequest *httpRequest,
							   int *size);

void createHttpResponseHeaderStatusLine(HttpResponse *httpResponse, 
										HttpRequest *httpRequest, 
										char *headbuf,
										int *length);

void provideUnknown(HttpRequest *httpRequest,
				 HttpResponse *httpResponse);

void provideHead(HttpRequest *httpRequest,
				 HttpResponse *httpResponse);

void provideGet(HttpRequest *httpRequest,
				HttpResponse *httpResponse);

void providePost(HttpRequest *httpRequest,
				 HttpResponse *httpResponse);

void provideOptions(HttpResponse *httpResponse);

#endif
