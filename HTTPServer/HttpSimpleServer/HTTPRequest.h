#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include <Windows.h>
#include "HTTPCommons.h"

#define ZERO_CONTENT_LENGTH 0
#define MISSING_CONTENT_LENGTH -1

/*
The structure contains the parsed information presented in the 
HTTP request line.
*/
typedef struct __http_request_line {
	enum HTTPMETHOD method;
	char *requestURI;
	char *httpVersion;
} HttpRequestLine;

/*
The structure contains the parsed information inside the 
HTTP request headers section.
*/
typedef struct __http_headers {
	char *contentType;
	int contentLength;
	char *charset;
} HttpHeaders;

/*
This strucutre contains the separate parts of an HTTP request - 
the status line, the headers and the body of the message.
*/
typedef struct __http_request {
	HttpRequestLine *requestLine;
	HttpHeaders *headers;
	char *entityBody;
} HttpRequest;

HttpRequest* parseHttpMessage(
	char *msgbuf, 
	int msglen);

HttpRequestLine* parseHttpStatusLine(
	char *msgbuf,
	int msglen,
	int *currentParsingPos);

HttpHeaders* parseHttpHeaders(
	char *msgbuf,
	int msglen,
	int *currentParsingPos);

char* acquireSubstring(
	char *buffer,
	int start,
	int end);

void freeHttpRequest(
	HttpRequest *httpRequest);

#endif
