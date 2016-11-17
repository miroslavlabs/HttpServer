#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

#include <Windows.h>
#include "HTTPCommons.h"
#include "HTTPRequest.h"
#include <stdio.h>

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
	char *body;
} HttpResponse;

void initStatusCodes();

char *provideFileContents(__in FILE *fp);

int sendHttpResponse(__in HttpRequest *httpRequest,
					 __in SOCKET *ClientSocket);

char *createHttpResponseHeader(__in HttpResponse *httpResponse,
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
