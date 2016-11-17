#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include <Windows.h>
#include "HTTPCommons.h"

#define NO_CONTENT 0
#define NO_CONTENT_LENGTH -1

typedef struct __http_request_line {
	enum HTTPMETHOD method;
	char *requestURI;
	char *httpVersion;
} HttpRequestLine;

typedef struct __http_headers {
	char *contentType;
	int contentLength;
	char *charset;
} HttpHeaders;

typedef struct __http_request {
	HttpRequestLine *requestLine;
	HttpHeaders *headers;
	char *entityBody;
} HttpRequest;

HttpRequest parseHttpMessage(
	__in char *msgbuf, 
	__in int msglen);

HttpRequestLine* parseHttpStatusLine(
	__in char *msgbuf,
	__in int msglen,
	__out int *currentParsingPos);

HttpHeaders* parseHttpHeaders(
	__in char *msgbuf,
	__in int msglen,
	__out int *currentParsingPos);

char* acquireSubstring(
	__in char *buffer,
	__in int start,
	__in int end);

void freeHttpRequest(
	__in HttpRequest *httpRequest);

#endif