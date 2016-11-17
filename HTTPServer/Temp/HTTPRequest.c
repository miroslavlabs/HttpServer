#include "HTTPRequest.h"
#include "HTTPCommons.h"

HttpRequest parseHttpMessage(
	__in char *msgbuf, 
	__in int msglen) {

	HttpRequest httpRequest;
	int currentParsingPos;

	httpRequest.requestLine = parseHttpStatusLine(msgbuf, msglen, &currentParsingPos);
	httpRequest.headers = parseHttpHeaders(msgbuf, msglen, &currentParsingPos);
	currentParsingPos++;
	httpRequest.entityBody = acquireSubstring(msgbuf, currentParsingPos, msglen);

	return httpRequest;
}

HttpRequestLine* parseHttpStatusLine(
	__in char *msgbuf,
	__in int msglen,
	__out int *currentParsingPos) {
	
	int i, lastPos;
	char *method, *httpVersion;
	HttpRequestLine *requestLine = (HttpRequestLine *) malloc(sizeof(HttpRequestLine));

	i = 0;
	lastPos = i;
	// Search for the first space character - the token before it
	// is the HTTP method.
	while(msgbuf[i] != SP) i++;

	// Acquire the method's string value.
	method = acquireSubstring(msgbuf, lastPos, i);

	// See which method was invoked. If the method is not recognized
	// assume the UNKNOW method and stop the parsing process.
	if(strcmp(method, "HEAD") == 0) {
		requestLine->method = HEAD;
	} else if(strcmp(method, "GET") == 0) {
		requestLine->method = GET;
	} else if(strcmp(method, "POST") == 0) {
		requestLine->method = POST;
	} else {
		requestLine->method = UNKNOWN_METHOD;
		goto ParsingDone;
	}

	// Remember the new starting position for the next item to be parsed.
	lastPos = ++i;
	// Search for the next item - the requestURI
	while(msgbuf[i] != SP) i++;
	requestLine->requestURI = acquireSubstring(msgbuf, lastPos, i);

	// Lastly - acquire the HTTP version.
	// Remember the new starting position for the next item to be parsed.
	lastPos = ++i;
	while(msgbuf[i] != CR && msgbuf[i + 1] != LF) i++;
	httpVersion = acquireSubstring(msgbuf, lastPos, i);

	if(strcmp(httpVersion, "HTTP/1.0") == 0) {
		requestLine->httpVersion = HTTP_1_0;
	} else if(strcmp(httpVersion, "HTTP/1.1") == 0) {
		requestLine->httpVersion = HTTP_1_1;
	} else {
		requestLine->httpVersion = UNKNOWN_VERSION;
	}

	// The length of the status line includes LF as well.
	*currentParsingPos = i + 1;

ParsingDone:
	free(method);
	free(httpVersion);

	return requestLine;
}

HttpHeaders* parseHttpHeaders(
	__in char *msgbuf,
	__in int msglen,
	__out int *currentParsingPos) {

	char *fieldName = NULL, *headers = NULL;
	int i, j;
	HttpHeaders* httpHeaders = NULL;

	httpHeaders = (HttpHeaders *) malloc(sizeof(HttpHeaders));
	// Using the start address of the headers to make the
	// calculations simpler and more understandable.
	headers = msgbuf + (*currentParsingPos) + 1;

	i = 0;
	while(headers[i] != CR && headers[i + 1] != LF) {
		j = i;
		while(headers[i] != ':') i++;
		fieldName = acquireSubstring(headers, j, i);
		// After the colon, the specification dictates that a space be present.
		// Increment i to skip over the space and the colon.
		i += 2;
		j = i;

		if(strcmp(fieldName, "Content-Type") == 0) {
			// Iterate until the end of line is reached or if the beggining of the
			// charset directive is enciuntered.
			while(headers[i] != ';' && (headers[i] != CR && headers[i + 1] != LF)) i++;
			httpHeaders->contentType = acquireSubstring(headers, j, i);

			if(headers[i] == ';') {
				// Find where the equals sign for the charset is and acquire the
				// string value of the charset.
				while(headers[i] != '=') i++;
				i++;
				j = i;

				while(headers[i] != CR && headers[i + 1] != LF) i++;
				httpHeaders->charset = acquireSubstring(headers, j, i);;
			}
		} else if(strcmp(fieldName, "Content-Length") == 0) {
			// Increment i until the end of the line is reached.
			while(headers[i] != CR && headers[i + 1] != LF) i++;
			httpHeaders->contentLength = atoi(acquireSubstring(headers, j, i));
		} else {
			// Reach the end of line and skip over the field value.
			while(headers[i] != CR && headers[i + 1] != LF) i++;
		}

		// Free only the field name buffer - he field value buffer may be used
		// in certain scenarios.
		free(fieldName);

		// Go to the next header - skip over the LF and move on to the next character.
		i += 2;
	}

	(*currentParsingPos) += i + 2;

	return httpHeaders;
}

char* acquireSubstring(
	__in char *buffer,
	__in int start,
	__in int end) {
	
	int parsedItemLength;
	char *substring;

	parsedItemLength = end - start + 1;
	substring = (char *) malloc(parsedItemLength * sizeof(char));
	memcpy(substring, buffer + start, parsedItemLength - 1);
	substring[parsedItemLength - 1] = '\0';

	return substring;
}