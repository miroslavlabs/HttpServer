#include "HTTPResponse.h"
#include <stdio.h>
#include "FileUtils.h"

#define DEFAULT_BUFLEN 1024

BOOL statusCodesInitialized = FALSE;

/*
This finction is responsible for initializing the status information
which will be returned with the HTTP response. The status information
will be initialized only the first time the function is invoked.
*/
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

	statusCodesInitialized = TRUE;
}

/*
This function is responsible for sending an HTTP response back to the client as an
answer to the HTTP request.

INPUT: 'httpRequest' is a pointer to an HttpRequest structure; 'ClientSocket' is 
a pointer to the client socket which will be used to send the response.

OUTPUT: The exit code of the operation.
*/
int sendHttpResponse(__in HttpRequest *httpRequest,
					 __in SOCKET *ClientSocket) {
	HttpResponse httpResponse;
	char *response_header = NULL, *response_body = NULL, *sendbuf = NULL;
	int iSendResult, result;
	int sendbuf_size;
	
	FILE *fd = NULL;

	// Initialize the status table.
	initStatusCodes();
	result = EXIT_OK;

	// Initialize the HTTP response strucutre.
	httpResponse.body = NULL;
	httpResponse.headers = NULL;
	httpResponse.httpVersion = NULL;
	httpResponse.status = NULL;

	/*
	Perform the action specified by the HTTP method if the method is supported.
	Should the method be unsupported, create an appropriate response.
	*/
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

	// Send the response to the client.
	sendbuf = createHttpResponseAsString(&httpResponse, httpRequest, &sendbuf_size);
	printf("%s\n", sendbuf);
	iSendResult = send(*ClientSocket, sendbuf, sendbuf_size, 0);
	if (iSendResult == SOCKET_ERROR) {
		printf("send failed with error: %d\n", WSAGetLastError());
		closesocket(*ClientSocket);
		WSACleanup();
		return EXIT_ERROR;
	}

	// Free the used resources.
	free(sendbuf);

	// Free the response headers.
	if(httpResponse.body != NULL) {
		free(httpResponse.headers);
	}

	return result;
}

/*
This function is responsible for creating the headers of the HTTP response message.

INPUT: 'httpResponse' is the a pointer to the structurw which will contain the HTTP response data;
'httpRequest' is a pointer to the structure which contains the HTTP request; 'size'is the total size
of the response message.

OUTPUT: A pointer to a char array, which contains the HTTP response message as a string.
*/
char *createHttpResponseAsString(__in HttpResponse *httpResponse,
							   __in HttpRequest *httpRequest,
							   __out int *size) {
	char *headbuf = NULL, *temp = NULL;
	int templen;
	
	headbuf = (char *) malloc(DEFAULT_BUFLEN * sizeof(char));

	// Create the status line of the response.
	createHttpResponseHeaderStatusLine(httpResponse, httpRequest, headbuf, size);
	// If no body exists for the response, do not create unnecessary headers.
	if (httpResponse->body != NULL) {
		sprintf(headbuf + *size, "Content-Type: %s;charset=%s\r\nContent-Length: %d\r\nConnection: close\r\n\r\n", 
			httpResponse->headers->contentType, httpResponse->headers->charset, httpResponse->headers->contentLength);

		// Copy the contents of the message body to the char array.
		*size = strlen(headbuf);
		templen = *size + httpResponse->headers->contentLength;
		temp = (char*) malloc(templen*sizeof(char*));
		memcpy(temp, headbuf, *size);
		memcpy(temp + *size, httpResponse->body, httpResponse->headers->contentLength);
		free(headbuf);
		headbuf = temp;
		*size = templen;
	} else {
		// Terminate the response message with CRLF.
		headbuf[(*size)++] = CR;
		headbuf[(*size)++] = LF;
		headbuf[*size] = '\0';
	}

	return headbuf;
}

/*
This function converts the status line of the HttpResponse structure into a string.

INPUT: 'httpResponse' is the a pointer to the structurw which will contain the HTTP response data;
'httpRequest' is a pointer to the structure which contains the HTTP request; 'headbuf'is the buffer which
will contain the parsed header information; 'length' will conatin the new length of the buffer
after the parsing is complete.
*/
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

	// Terminate the status line with CRLF.
	headbuf[headbuflen++] = CR;
	headbuf[headbuflen++] = LF;
	headbuf[headbuflen] = '\0';
	
	*length = headbuflen;
}

/*
This function creates an HttpResponse for an unsupported HTTP method.

INPUT: 'httpRequest' is a pointer to an HttpRequest structure; 'httpResponse' is a
pointer to an HttpResponse structure whose fields will be created accordingly.
*/
void provideUnknown(__in HttpRequest *httpRequest,
				 __out HttpResponse *httpResponse) {
	httpResponse->httpVersion = httpRequest->requestLine->httpVersion;
	httpResponse->status = &StatusCodesTable.NotImplemented;
}

/*
This function creates an HttpResponse for an the HTTP HEAD method.

INPUT: 'httpRequest' is a pointer to an HttpRequest structure; 'httpResponse' is a
pointer to an HttpResponse structure whose fields will be created accordingly.
*/
void provideHead(__in HttpRequest *httpRequest,
				 __out HttpResponse *httpResponse) {

	FILE *fd = NULL;

	httpResponse->httpVersion = httpRequest->requestLine->httpVersion;
	
	// Open the file for reading.
	fd = fopen(httpRequest->requestLine->requestURI, "r");

	// If the file cannot be open, then the resource doesn't exist.
	if(!fd) {
		/*
		If the file doesn't exist, send no headers - just the status line, stating
		that the requested resource cannot be found.
		*/
		httpResponse->status = &StatusCodesTable.NotFound;
	} else {
		// Declare that the file exists and acquire the necessary information about the resource.
		httpResponse->status = &StatusCodesTable.OK;
		httpResponse->body = EMPTY_STRING;
		httpResponse->headers = (HttpHeaders *) malloc(sizeof(HttpHeaders));
		/*
		Stick to the header information from the request or return default
		information in case the specified headers were not set in the request.
		*/
		httpResponse->headers->contentType = httpRequest->headers->contentType == NULL ? "text/html" : httpRequest->headers->contentType;
		httpResponse->headers->charset = httpRequest->headers->charset  == NULL ? "UTF-8" : httpRequest->headers->charset;

		// Acquire the length of the file.
		fseek(fd, 0L, SEEK_END);
		httpResponse->headers->contentLength = ftell(fd);
	}

	if(fd) {
		fclose(fd);
	}
}

/*
This function creates an HttpResponse for an the HTTP GET method.

INPUT: 'httpRequest' is a pointer to an HttpRequest structure; 'httpResponse' is a
pointer to an HttpResponse structure whose fields will be created accordingly.
*/
void provideGet(__in HttpRequest *httpRequest,
				 __out HttpResponse *httpResponse) {
	FILE *fd = NULL;
	// The GET method uses the same status line and headers created when responding to the HEAD method.
	provideHead(httpRequest, httpResponse);
	// Acquire the file's contents and add them to the response.
	if(httpResponse->status->statusCode != StatusCodesTable.NotFound.statusCode) {
		fd = fopen(httpRequest->requestLine->requestURI, "r");
		if (fd) {
			httpResponse->body = provideFileContents(fd);
			fclose(fd);
		}
	}
}

/*
This function creates an HttpResponse for an the HTTP POST method.

INPUT: 'httpRequest' is a pointer to an HttpRequest structure; 'httpResponse' is a
pointer to an HttpResponse structure whose fields will be created accordingly.
*/
void providePost(__in HttpRequest *httpRequest,
				 __out HttpResponse *httpResponse) {
	
	FILE *fd = NULL;
    int result;
	
	/*
	When POST is recieved, it is necessary to verify the the Content-Length header was
	provided and to verify wether the content length that differs from zero. Based on
	the given scenario, the appropriate response will be created.
	*/
	if(httpRequest->headers->contentLength == ZERO_CONTENT_LENGTH) {
		httpResponse->status = &StatusCodesTable.NoContent;
	
	} else if(httpRequest->headers->contentLength == MISSING_CONTENT_LENGTH) {
		httpResponse->status = &StatusCodesTable.BadRequest;
	
	} else if(httpRequest->headers->contentType == NULL) {
		/*
		If the content type is missing, the server cannot return the
		data in a format that will be understood by the client.
		*/
		httpResponse->status = &StatusCodesTable.BadRequest;
	
	} else {
		/*
		If the file resource exsits, append the contents of the HTTP request body to the
		end of the file. Otherwise, respond that the resource cannot be found.
		*/
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

/*
This function creates an HttpResponse for an the HTTP OPTIONS method.
OPTIONS will return only the supported HTTP methods and supported MIME types.

INPUT: 'httpRequest' is a pointer to an HttpRequest structure; 'httpResponse' is a
pointer to an HttpResponse structure whose fields will be created accordingly.
*/
void provideOptions(__in HttpRequest *httpRequest,
				 __out HttpResponse *httpResponse) {
	httpResponse->status = &StatusCodesTable.OK;

	// Specify the supported HTTP methods and supported MIME types.
	httpResponse->body = "This server supports the following methods: HEAD, GET, PUT, OPTIONS.\n" \
		"The following MIME types are supported: text/html, text/plain";

	// Specify the body's length and MIME type.
	httpResponse->headers = (HttpHeaders *) malloc(sizeof(HttpHeaders));
	httpResponse->headers->contentType = "text/plain";
	httpResponse->headers->charset = "UTF-8";
	httpResponse->headers->contentLength = strlen(httpResponse->body);
}