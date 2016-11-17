#include <Windows.h>

enum HTTPMETHOD {
	HEAD, GET, POST, UNKNOWN_METHOD
};

enum HTTPVERSION {
	HTTP_1_0, HTTP_1_1, UNKNOWN_VERSION
};

typedef struct __http_request_line {
	enum HTTPMETHOD method;
	char *requestURI;
	enum HTTPVERSION httpVersion;
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