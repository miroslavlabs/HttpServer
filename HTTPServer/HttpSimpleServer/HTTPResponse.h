#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

#include <Windows.h>
#include "HTTPCommons.h"
#include "HTTPRequest.h"

typedef struct __status {
	char *statusCode;
	char *reasonPhrase;
} Status;

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
} StatusCodesTable;

typedef struct __http_response {
	char *httpVersion;
	Status *status;
	HttpHeaders *headers;
} HttpResponse;

void initStatusCodes();

int sendHttpResponse(HttpRequest httpRequest, SOCKET ClientSocket);
char *createHttpResponseHeader(HttpResponse httpResponse, HttpRequest httpRequest);
void createHttpResponseHeaderStatusLine(HttpResponse httpResponse, HttpRequest httpRequest, char *headbuf, int *length);

#endif
