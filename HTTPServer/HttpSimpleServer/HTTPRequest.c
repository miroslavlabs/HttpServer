#include "HTTPRequest.h"
#include "HTTPCommons.h"

/*
This parses the acquired HTTP request message and stores the parsed
data in an HttpRequest structure. If the provided method field in the
request is not recognized as an internally supported HTTP method, then
the parsing process is stoped and the remaining fields are set to ne NULL.

INPUT: 'msgbuf' is a pointer to the buffer which stores the HTTP request;
'msglen' is the length of the request message.

OUTPUT: A pointer to an HttpRequest structre where the parsed message data
is stored.
*/
HttpRequest* parseHttpMessage(
	char *msgbuf, 
	int msglen) {

	HttpRequest *httpRequest;
	int currentParsingPos;

	httpRequest = (HttpRequest*)malloc(sizeof(HttpRequest));
	httpRequest->requestLine = NULL;
	httpRequest->headers = NULL;
	httpRequest->entityBody = NULL;
	
	/*
	Parse the status line first. If the method is recognized as an internal one,
	parse the remainder of the HTTP request.
	*/
	httpRequest->requestLine = parseHttpStatusLine(msgbuf, msglen, &currentParsingPos);
	if(httpRequest->requestLine->method != UNKNOWN_METHOD) {
		httpRequest->headers = parseHttpHeaders(msgbuf, msglen, &currentParsingPos);
		currentParsingPos++;
		httpRequest->entityBody = acquireSubstring(msgbuf, currentParsingPos, msglen);
		httpRequest->entityBody[msglen - 1] = '\0';
	}

	return httpRequest;
}

/*
This method is responsible for parsing the HTTP status line in the form
"HTTP method SP resource URI SP HTTP version CRLF". The parsed segments
are stored separately in the structure HttpRequestLine.

INPUT: 'msgbuf' is a pointer to the buffer which stores the HTTP request;
'msglen' is the length of the request message; 'currentParsingPos' will store
the position of the last character in the request status line.

OUTPUT: A pointer to an HttpRequestLine structre which contains the parsed
status line information.
*/
HttpRequestLine* parseHttpStatusLine(
	char *msgbuf,
	int msglen,
	int *currentParsingPos) {
	
	int i, lastPos, requestUriLen;
	char *method  = NULL, *requestURI;
	HttpRequestLine *requestLine = NULL;

	i = 0;
	lastPos = i;

	requestLine = (HttpRequestLine *) malloc(sizeof(HttpRequestLine));
	/*
	Search for the first space character - the token before it
	is the HTTP method.
	*/
	while(i < msglen && msgbuf[i] != SP) i++;

	// Acquire the method's string value.
	method = acquireSubstring(msgbuf, lastPos, i);

	/*
	See which HTTP method was specified. If the method is not recognized,
	assume the UNKNOW method and stop the parsing process.
	*/
	if(strcmp(method, "HEAD") == 0) {
		requestLine->method = HEAD;
	} else if(strcmp(method, "GET") == 0) {
		requestLine->method = GET;
	} else if(strcmp(method, "POST") == 0) {
		requestLine->method = POST;
	} else if(strcmp(method, "OPTIONS") == 0) {
		requestLine->method = OPTIONS;
	} else {
		requestLine->method = UNKNOWN_METHOD;
	}

	// Remember the new starting position for the next item to be parsed.
	lastPos = ++i;
	// Search for the next item - the requestURI
	while(i < msglen && msgbuf[i] != SP) i++;
	requestURI = acquireSubstring(msgbuf, lastPos, i);
	/* 
	To find the requested resource, add a dot before the backslash to specify that
	the path is relative to the current folder. Therefore,Add two to the last position - 
	one character for the dot and one for the terminating null character.
	*/
	requestUriLen = (strlen(requestURI) + 2);
	requestLine->requestURI = (char *) malloc(requestUriLen * sizeof(char));

	requestLine->requestURI[0] = '.';
	memcpy(requestLine->requestURI + 1, requestURI, strlen(requestURI));
	requestLine->requestURI[requestUriLen - 1] = '\0';

	// Lastly - acquire the HTTP version.
	// Remember the new starting position for the next item to be parsed.
	lastPos = ++i;

	while(i < msglen - 1 && msgbuf[i] != CR && msgbuf[i + 1] != LF) i++;
	requestLine->httpVersion = acquireSubstring(msgbuf, lastPos, i);

	// The length of the status line includes LF as well.
	*currentParsingPos = i + 1;

	free(method);

	return requestLine;
}

/*
This method is responsible for parsing the HTTP headers inside the HTTP request
message. The accpeted headers are the Content-Type and Content-Length message,
with th inclusion of the charset attribute. The parsed headers and attributes
are stored in an HttpHeaders structure.

INPUT: 'msgbuf' is a pointer to the buffer which stores the HTTP request;
'msglen' is the length of the request message; 'currentParsingPos' will store
the position of the last character in the headers section.

OUTPUT: A pointer to an HttpHeaders structre which contains the parsed
headers data.
*/
HttpHeaders* parseHttpHeaders(
	char *msgbuf,
	int msglen,
	int *currentParsingPos) {

	char *fieldName = NULL, *headers = NULL;
	int i, pos;
	HttpHeaders* httpHeaders = NULL;

	// Initialize the parameters of the structure with default null
	// values.
	httpHeaders = (HttpHeaders *) malloc(sizeof(HttpHeaders));
	httpHeaders->contentType = NULL;
	httpHeaders->contentLength = MISSING_CONTENT_LENGTH;
	httpHeaders->charset = NULL;

	/*
	Using the start address of the headers to make the
	calculations simpler and more understandable.
	*/
	headers = msgbuf + (*currentParsingPos) + 1;

	i = 0;
	/*
	Begin the process of parsing the headers of the HTTP request message.
	The parsing finishes when a line, which contains just CRLF, is encountered.
	*/
	while(i < msglen - 1 && headers[i] != CR && headers[i + 1] != LF) {
		pos = i;
		while(headers[i] != ':') i++;
		fieldName = acquireSubstring(headers, pos, i);
		/*
		After the colon, the specification dictates that a space be present.
		Increment i to skip over the space and the colon.
		*/
		i += 2;
		pos = i;

		if(strcmp(fieldName, "Content-Type") == 0) {
			/*
			Iterate until the end of line is reached or if the beggining of the
			charset directive is encountered.
			*/
			while(i + 1 < msglen && headers[i] != ';' && (headers[i] != CR && headers[i + 1] != LF)) i++;
			httpHeaders->contentType = acquireSubstring(headers, pos, i);

			if(headers[i] == ';') {
				/*
				Find where the equals sign for the charset is and acquire the
				string value of the charset.
				*/
				while(i < msglen && headers[i] != '=') i++;
				i++;
				pos = i;

				// Acquire the charset directive's value.
				while(i + 1 < msglen && headers[i] != CR && headers[i + 1] != LF) i++;
				httpHeaders->charset = acquireSubstring(headers, pos, i);
			}
		} else if(strcmp(fieldName, "content-length") == 0 || 
			strcmp(fieldName, "Content-Length") == 0) {
			// Increment i until the end of the line is reached.
			while(i + 1 < msglen && headers[i] != CR && headers[i + 1] != LF) i++;
			httpHeaders->contentLength = atoi(acquireSubstring(headers, pos, i));
		} else {
			/* 
			If an unsupported header field is encountered, iterate until the end of the line,
			skipping over the field value.
			*/
			while(i + 1 < msglen && headers[i] != CR && headers[i + 1] != LF) i++;
		}

		/*
		Free only the field name buffer - he field value buffer may be used
		in certain scenarios.
		*/
		free(fieldName);

		// Go to the next header - skip over the LF and move on to the next character.
		i += 2;
	}

	(*currentParsingPos) += i + 2;

	return httpHeaders;
}

/*
Acquire a substring from the provided char buffer, delimited by a start and end poisition
inclusive.

INPUT: 'buffer'is a pointer to the char array; 'start' is the beggining position of the
substring inside the char array; 'end' is the end position of the substring.

OUTPUT: A pointer to the beggining of the substring char array.
*/
char* acquireSubstring(
	char *buffer,
	int start,
	int end) {
	
	int parsedItemLength;
	char *substring;

	parsedItemLength = end - start + 1;
	substring = (char *) malloc(parsedItemLength * sizeof(char));
	memcpy(substring, buffer + start, parsedItemLength - 1);
	substring[parsedItemLength - 1] = '\0';

	return substring;
}

/*
This method frees the resources acquired by the creation of the HttpRequest
structure.

INPUT: A pointer to the HttpRequest strucure, whose resources are to be
deallocated.
*/
void freeHttpRequest(
	HttpRequest *httpRequest) {
	if(httpRequest->entityBody != NULL)
		free(httpRequest->entityBody);

	free(httpRequest->requestLine->httpVersion);
	free(httpRequest->requestLine->requestURI);

	if(httpRequest->headers != NULL) {
		if(httpRequest->headers->contentType != NULL)
			free(httpRequest->headers->contentType);

		if(httpRequest->headers->charset != NULL)
			free(httpRequest->headers->charset);
	}
}
