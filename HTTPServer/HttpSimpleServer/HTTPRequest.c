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
	httpRequest.entityBody[msglen - 1] = '\0';

	return httpRequest;
}

HttpRequestLine* parseHttpStatusLine(
	__in char *msgbuf,
	__in int msglen,
	__out int *currentParsingPos) {
	
	int i, lastPos, requestUriLen;
	char *method  = NULL, *requestURI;
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
	requestURI = acquireSubstring(msgbuf, lastPos, i);
	requestUriLen = (strlen(requestURI) + 2);
	requestLine->requestURI = (char *) malloc(requestUriLen * sizeof(char));
	requestLine->requestURI[0] = '.';
	memcpy(requestLine->requestURI + 1, requestURI, strlen(requestURI));
	requestLine->requestURI[requestUriLen - 1] = '\0';

	// Lastly - acquire the HTTP version.
	// Remember the new starting position for the next item to be parsed.
	lastPos = ++i;
	while(msgbuf[i] != CR && msgbuf[i + 1] != LF) i++;
	requestLine->httpVersion = acquireSubstring(msgbuf, lastPos, i);

	// The length of the status line includes LF as well.
	*currentParsingPos = i + 1;

ParsingDone:
	free(method);

	return requestLine;
}

HttpHeaders* parseHttpHeaders(
	__in char *msgbuf,
	__in int msglen,
	__out int *currentParsingPos) {

	char *fieldName = NULL, *headers = NULL;
	int i, pos;
	HttpHeaders* httpHeaders = NULL;

	// Initialize the parameters of the structure with default null
	// values.
	httpHeaders = (HttpHeaders *) malloc(sizeof(HttpHeaders));
	httpHeaders->contentType = NULL;
	httpHeaders->contentLength = NO_CONTENT_LENGTH;
	httpHeaders->charset = NULL;

	// Using the start address of the headers to make the
	// calculations simpler and more understandable.
	headers = msgbuf + (*currentParsingPos) + 1;

	i = 0;
	while(i < msglen - 1 && headers[i] != CR && headers[i + 1] != LF) {
		pos = i;
		while(headers[i] != ':') i++;
		fieldName = acquireSubstring(headers, pos, i);
		// After the colon, the specification dictates that a space be present.
		// Increment i to skip over the space and the colon.
		i += 2;
		pos = i;

		if(strcmp(fieldName, "Content-Type") == 0) {
			// Iterate until the end of line is reached or if the beggining of the
			// charset directive is enciuntered.
			while(i + 1 < msglen && headers[i] != ';' && (headers[i] != CR && headers[i + 1] != LF)) i++;
			httpHeaders->contentType = acquireSubstring(headers, pos, i);

			if(headers[i] == ';') {
				// Find where the equals sign for the charset is and acquire the
				// string value of the charset.
				while(i < msglen && headers[i] != '=') i++;
				i++;
				pos = i;

				while(i + 1 < msglen && headers[i] != CR && headers[i + 1] != LF) i++;
				httpHeaders->charset = acquireSubstring(headers, pos, i);
			}
		} else if(strcmp(fieldName, "content-length") == 0 || 
			strcmp(fieldName, "Content-Length") == 0) {
			// Increment i until the end of the line is reached.
			while(i + 1 < msglen && headers[i] != CR && headers[i + 1] != LF) i++;
			httpHeaders->contentLength = atoi(acquireSubstring(headers, pos, i));
		} else {
			// Reach the end of line and skip over the field value.
			while(i + 1 < msglen && headers[i] != CR && headers[i + 1] != LF) i++;
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

void freeHttpRequest(
	__in HttpRequest *httpRequest) {
	if(httpRequest->entityBody != NULL)
		free(httpRequest->entityBody);

	free(httpRequest->requestLine->httpVersion);
	free(httpRequest->requestLine->requestURI);

	if(httpRequest->headers->contentType != NULL)
		free(httpRequest->headers->contentType);

	if(httpRequest->headers->charset != NULL)
		free(httpRequest->headers->charset);
}