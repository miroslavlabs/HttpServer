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

char* provideFileContents(__in FILE *fp) {	
	long file_size;
	char *buffer;

	// First, the file size needs to be discovered.
	// To do that we seek the end of the file and then we rewind the
	// file pointer to point to the beginning of the file again.
	fseek(fp , 0L , SEEK_END);
	file_size = ftell(fp);
	rewind(fp);

	// Allocate memory for entire content.
	buffer = (char*)calloc(file_size + 1, sizeof(char));
	if(!buffer) {
		return NULL;
	}

	// Copy the file into the bufffer.
	fread(buffer , file_size, 1 , fp);

	buffer[file_size] = '\0';

	// FIXME - make a valid status code.
	return buffer;
}

int fileExists(const char *filename)
{
   FILE *fp = fopen (filename, "r");
   if (fp!=NULL) fclose (fp);
   return (fp!=NULL);
}

int sendHttpResponse(__in HttpRequest *httpRequest,
					 __in SOCKET *ClientSocket) {
	HttpResponse httpResponse;
	char *response_header = NULL, *response_body = NULL, *sendbuf = NULL;
	int iSendResult, result;
	int sendbuf_size;
	
	FILE *fd = NULL;

	initStatusCodes();
	result = EXIT_OK;

	httpResponse.body = NULL;
	httpResponse.headers = NULL;
	httpResponse.httpVersion = NULL;
	httpResponse.status = NULL;

	if(httpRequest->requestLine->method == UNKNOWN_METHOD) {
		provideUnknown(httpRequest, &httpResponse);
	} else {
		if(httpRequest->requestLine->method == GET) {
			provideGet(httpRequest, &httpResponse);
		} else if (httpRequest->requestLine->method == HEAD) {
			provideHead(httpRequest, &httpResponse);
		} else if (httpRequest->requestLine->method == POST) {
			providePost(httpRequest, &httpResponse);
		} else if(httpRequest->requestLine->method == OPTIONS) {
			provideOptions(httpRequest, &httpResponse);
		}
	}
	sendbuf = createHttpResponseHeader(&httpResponse, httpRequest, &sendbuf_size);
	printf("%s\n", sendbuf);
	iSendResult = send(*ClientSocket, sendbuf, sendbuf_size, 0);
	if (iSendResult == SOCKET_ERROR) {
		printf("send failed with error: %d\n", WSAGetLastError());
		closesocket(*ClientSocket);
		WSACleanup();
		return EXIT_ERROR;
	}

	//Free the used resources.
	free(sendbuf);

	if(httpResponse.body != NULL) {
		free(httpResponse.headers);
	}

	return result;
}

char *createHttpResponseHeader(__in HttpResponse *httpResponse,
							   __in HttpRequest *httpRequest,
							   __out int *size) {
	char *headbuf = NULL, *temp = NULL;
	int templen;
	
	headbuf = (char *) malloc(DEFAULT_BUFLEN * sizeof(char));

	// Create the status line of the response
	createHttpResponseHeaderStatusLine(httpResponse, httpRequest, headbuf, size);
	if (httpResponse->body != NULL) {
		sprintf(headbuf + *size, "Content-Type: %s;charset=%s\r\nContent-Length: %d\r\nConnection: close\r\n\r\n", 
			httpResponse->headers->contentType, httpResponse->headers->charset, httpResponse->headers->contentLength);

		// new code
		*size = strlen(headbuf);
		templen = *size + httpResponse->headers->contentLength;
		temp = (char*) malloc(templen*sizeof(char*));
		memcpy(temp, headbuf, *size);
		memcpy(temp + *size, httpResponse->body, httpResponse->headers->contentLength);
		free(headbuf);
		headbuf = temp;
		*size = templen;
	} else {
		headbuf[(*size)++] = CR;
		headbuf[(*size)++] = LF;
		headbuf[*size] = '\0';
	}

	return headbuf;
}

void createHttpResponseHeaderStatusLine(__in HttpResponse *httpResponse, 
										__in HttpRequest *httpRequest, 
										__in char *headbuf,
										__out int *length) {
	int headbuflen, itemLength;
	headbuflen = 0;

	//Add the HTTP version field.
	itemLength = strlen(httpRequest->requestLine->httpVersion);
	memcpy(headbuf, httpRequest->requestLine->httpVersion, itemLength);

	headbuflen += itemLength;
	headbuf[headbuflen] = SP;
	headbuflen++;

	// Add the Status Code.
	itemLength = strlen(httpResponse->status->statusCode);
	memcpy(headbuf + headbuflen, httpResponse->status->statusCode, itemLength);

	headbuflen += itemLength;
	headbuf[headbuflen] = SP;
	headbuflen++;

	// Add the reason phrase.
	itemLength = strlen(httpResponse->status->reasonPhrase);
	memcpy(headbuf + headbuflen, httpResponse->status->reasonPhrase, itemLength);
	headbuflen += itemLength;
	headbuf[headbuflen++] = CR;
	headbuf[headbuflen++] = LF;
	headbuf[headbuflen] = '\0';
	
	*length = headbuflen;
}

void provideUnknown(__in HttpRequest *httpRequest,
				 __out HttpResponse *httpResponse) {
	httpResponse->httpVersion = httpRequest->requestLine->httpVersion;
	httpResponse->status = &StatusCodesTable.NotImplemented;
}

void provideHead(__in HttpRequest *httpRequest,
				 __out HttpResponse *httpResponse) {

	FILE *fd = NULL;

	httpResponse->httpVersion = httpRequest->requestLine->httpVersion;
	
	fd = fopen(httpRequest->requestLine->requestURI, "r");
	// If the file cannot be open, then the resource doesn't exist.
	if(!fd) {
		// If the file doesn't exist, send no headers - just the status line.
		httpResponse->status = &StatusCodesTable.NotFound;
	} else {
		httpResponse->status = &StatusCodesTable.OK;
		httpResponse->body = EMPTY_STRING;
		httpResponse->headers = (HttpHeaders *) malloc(sizeof(HttpHeaders));
		// Stick to the header information from the request or return default
		// information in case the specified headers were not set in the request.
		httpResponse->headers->contentType = httpRequest->headers->contentType == NULL ? "text/html" : httpRequest->headers->contentType;
		httpResponse->headers->charset = httpRequest->headers->charset  == NULL ? "UTF-8" : httpRequest->headers->charset;

		fseek(fd, 0L, SEEK_END);
		httpResponse->headers->contentLength = ftell(fd);
	}

	if(fd) {
		fclose(fd);
	}
}

void provideGet(__in HttpRequest *httpRequest,
				 __out HttpResponse *httpResponse) {
	FILE *fd = NULL;
	provideHead(httpRequest, httpResponse);
	if(httpResponse->status->statusCode != StatusCodesTable.NotFound.statusCode) {
		fd = fopen(httpRequest->requestLine->requestURI, "r");
		if (fd) {
			httpResponse->body = provideFileContents(fd);
			fclose(fd);
		}
	}
}

void providePost(__in HttpRequest *httpRequest,
				 __out HttpResponse *httpResponse) {
	
	FILE *fd = NULL;
    int result;
	
	// When POST is recieved, it is necessary to verify the the Content-Length header was
	// provided and to verify wether it has length that differs from zero.
	if(httpRequest->headers->contentLength == ZERO_CONTENT_LENGTH) {
		httpResponse->status = &StatusCodesTable.NoContent;
	
	} else if(httpRequest->headers->contentLength == MISSING_CONTENT_LENGTH) {
		httpResponse->status = &StatusCodesTable.BadRequest;
	
	} else if(httpRequest->headers->contentType == NULL) {
		// if the content type is missing, the server cannot return the
		// data in a format that will be understood by the client.
		httpResponse->status = &StatusCodesTable.BadRequest;
	
	} else {
		// The first time we process the POST request, we have not yet written
		// anything to the file. For now, we assume that all is correct.
		if(fileExists(httpRequest->requestLine->requestURI)) {
			httpResponse->status = &StatusCodesTable.OK;
			fd = fopen(httpRequest->requestLine->requestURI, "a");
			if (fd) {
				fputs(httpRequest->entityBody, fd);
				fclose(fd);
			}
		} else {
			httpResponse->status = &StatusCodesTable.NotFound;
		}
	}
}

void provideOptions(__in HttpRequest *httpRequest,
				 __out HttpResponse *httpResponse) {
	// Options will return only the supported HHTP methods and supported MIME types.
	httpResponse->status = &StatusCodesTable.OK;

	httpResponse->body = "This server supports the following methods: HEAD, GET, PUT, OPTIONS.\n" \
		"The following MIME types are supported: text/html, text/plain";

	httpResponse->headers = (HttpHeaders *) malloc(sizeof(HttpHeaders));
	httpResponse->headers->contentType = "text/plain";
	httpResponse->headers->charset = "UTF-8";
	httpResponse->headers->contentLength = strlen(httpResponse->body);
}