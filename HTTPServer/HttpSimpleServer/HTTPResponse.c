#include "HTTPResponse.h"
#include <stdio.h>

#define DEFAULT_BUFLEN 1024

BOOL statusCodesInitialized = FALSE;

void initStatusCodes() {
	if(statusCodesInitialized == TRUE) {
		return;
	}

	StatusCodesTable.OK.statusCode = "200";
	StatusCodesTable.OK.reasonPhrase = "OK";

	StatusCodesTable.Created.statusCode = "201";
	StatusCodesTable.Created.reasonPhrase = "Created";

	StatusCodesTable.NoContent.statusCode = "204";
	StatusCodesTable.NoContent.reasonPhrase = "No Content";

	StatusCodesTable.BadRequest.statusCode = "400";
	StatusCodesTable.BadRequest.reasonPhrase = "Bad Request";

	StatusCodesTable.NotFound.statusCode = "404";
	StatusCodesTable.NotFound.reasonPhrase = "Not Found";

	StatusCodesTable.NotImplemented.statusCode = "501";
	StatusCodesTable.NotImplemented.reasonPhrase = "Not Implemented";
}

int sendHttpResponse(HttpRequest httpRequest, SOCKET ClientSocket) {
	HttpResponse httpResponse;
	HttpHeaders *headers;
	char *sendbuf = NULL;
	int sendbuflen, iSendResult, result;
	
	FILE *fd = NULL;
	FILE *fp;

	initStatusCodes();
	result = EXIT_OK;

	httpResponse.httpVersion = httpRequest.requestLine->httpVersion;
	headers = (HttpHeaders *) malloc(sizeof(HttpHeaders));
	httpResponse.headers = headers;
	// Stick to the header information from the request or return default
	// information in case the specified headers were not set in the request.
	headers->contentType = httpRequest.headers->contentType == NULL ? "text/plain" : httpRequest.headers->contentType;
	headers->charset = httpRequest.headers->charset  == NULL ? "UTF-8" : httpRequest.headers->charset;
	headers->contentLength = NO_CONTENT;

	if(httpRequest.requestLine->method == UNKNOWN_METHOD) {
		httpResponse.status = &StatusCodesTable.NotImplemented;
	} else {
		// Open both for reading and for writing.
		fd = fopen(httpRequest.requestLine->requestURI, "a+");
		// If the file cannot be open, then the resource doesn't exist.
		if(fd <= 0) {
			httpResponse.status = &StatusCodesTable.NotFound;
		} else {
			// If the method is understood as a GET or HEAD, then the status code will be set as OK.
			if(httpRequest.requestLine->method == GET || httpRequest.requestLine->method == HEAD) {
				httpResponse.status = &StatusCodesTable.OK;
				// Since GET/HEAD weill be returning a resource or info about the resource,
				// acquire the length of that resource.
				fseek(fd, 0L, SEEK_END);
				headers->contentLength = ftell(fd);
				fseek(fd, 0L, SEEK_SET);
			} else if(httpRequest.requestLine->method == POST) {
				// When POST is recieved, it is necessary to verify the the Content-Length header was
				// provided and to verify wether it has length that differs from zero.
				if(httpRequest.headers->contentLength == NO_CONTENT) {
					httpResponse.status = &StatusCodesTable.NoContent;
				} else if(httpRequest.headers->contentLength == NO_CONTENT_LENGTH) {
					httpResponse.status = &StatusCodesTable.BadRequest;
				} else if(httpRequest.headers->contentType == NULL) {
					// if the content type is missing, the server cannot return the
					// data in a format that will be understood by the client.
					httpResponse.status = &StatusCodesTable.BadRequest;
				} else {
					// The first time we process the POST request, we have not yet written
					// anything to the file. For now, we assume that all is correct.
					httpResponse.status = &StatusCodesTable.OK;
				}
			}
		}
	}
	
	// Get the header and then send it to the client.
	sendbuf = createHttpResponseHeader(httpResponse, httpRequest);
	 
	// Return the header part to the client.
	iSendResult = send( ClientSocket, sendbuf, strlen(sendbuf), 0);
	if (iSendResult == SOCKET_ERROR) {
		printf("send failed with error: %d\n", WSAGetLastError());
		result = EXIT_ERROR;
		goto CleanUp;
	}

	if(fd > 0) {
		// Return the requested resource when the GET method is used.
		if(httpRequest.requestLine->method == GET) {
			while(fgets(sendbuf, DEFAULT_BUFLEN, fd)) {
				iSendResult = send( ClientSocket, sendbuf, strlen(sendbuf), 0);
				if (iSendResult == SOCKET_ERROR) {
					printf("send failed with error: %d\n", WSAGetLastError());
					break;
				}
			}
		} else if(httpRequest.requestLine->method == POST) {
			// Place the data sent over in the file.
			fputs(httpRequest.entityBody, fd);
		}
	}

	printf("Bytes sent: %d\n", iSendResult);

CleanUp:
	// Close the file.
	if(fd > 0) {
		fclose(fd);
	}

	free(headers);
	free(sendbuf);

	return result;
}

char *createHttpResponseHeader(HttpResponse httpResponse, HttpRequest httpRequest) {
	char *headbuf = NULL;
	int headbuflen;

	headbuf = (char *) malloc(DEFAULT_BUFLEN * sizeof(char));

	// Create the status line of the response
	createHttpResponseHeaderStatusLine(httpResponse, httpRequest, headbuf, &headbuflen);
	sprintf(headbuf + headbuflen + 1, "Content-Type: %s;charset=%s\r\nContent-Length: %d\r\n\r\n\0", 
		httpResponse.headers->contentType, httpResponse.headers->charset, httpResponse.headers->contentLength);

	return headbuf;
}

void createHttpResponseHeaderStatusLine(HttpResponse httpResponse, HttpRequest httpRequest, char *headbuf, int *length) {
	int headbuflen, itemLength;
	headbuflen = 0;

	//Add the HTTP version field.
	itemLength = strlen(httpRequest.requestLine->httpVersion);
	memcpy(headbuf, httpRequest.requestLine->httpVersion, itemLength);

	headbuflen += itemLength;
	headbuf[headbuflen] = SP;
	headbuflen++;

	// Add the Status Code.
	itemLength = strlen(httpResponse.status->statusCode);
	memcpy(headbuf + headbuflen, httpResponse.status->statusCode, itemLength);

	headbuflen += itemLength;
	headbuf[headbuflen] = SP;
	headbuflen++;

	// Add the reason phrase.
	itemLength = strlen(httpResponse.status->reasonPhrase);
	memcpy(headbuf + headbuflen, httpResponse.status->reasonPhrase, itemLength);
	headbuflen += itemLength;
	headbuf[headbuflen++] = CR;
	headbuf[headbuflen] = LF;
	
	*length = headbuflen;
}