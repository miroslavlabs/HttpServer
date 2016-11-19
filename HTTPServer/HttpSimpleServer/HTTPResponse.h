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
} StatusCodesTable;

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

int sendHttpResponse(__in HttpRequest *httpRequest,
					 __in SOCKET *ClientSocket);

char *createHttpResponseAsString(__in HttpResponse *httpResponse,
							   __in HttpRequest *httpRequest,
							   __out int *size);

void createHttpResponseHeaderStatusLine(__in HttpResponse *httpResponse, 
										__in HttpRequest *httpRequest, 
										__in char *headbuf,
										__out int *length);

void provideUnknown(__in HttpRequest *httpRequest,
				 __out HttpResponse *httpResponse);

void provideHead(__in HttpRequest *httpRequest,
				 __out HttpResponse *httpResponse);

void provideGet(__in HttpRequest *httpRequest,
				__out HttpResponse *httpResponse);

void providePost(__in HttpRequest *httpRequest,
				 __out HttpResponse *httpResponse);

void provideOptions(__in HttpRequest *httpRequest,
				 __out HttpResponse *httpResponse);

#endif
