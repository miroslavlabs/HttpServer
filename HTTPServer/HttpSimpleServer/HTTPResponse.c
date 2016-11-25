#include "HTTPResponse.h"
#include <stdio.h>
#include "FileUtils.h"
#include <Windows.h>

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
int sendHttpResponse(HttpRequest *httpRequest,
					 SOCKET *ClientSocket) {
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
			provideOptions(&httpResponse);
		}
	}

	// Send the response to the client.
	sendbuf = createHttpResponseAsString(&httpResponse, httpRequest, &sendbuf_size);
	printf("%s\n", sendbuf);
	iSendResult = send(*ClientSocket, sendbuf, sendbuf_size, 0);
	if (iSendResult == SOCKET_ERROR) {
		printf("send failed with error: %d\n", WSAGetLastError());
		closesocket(*ClientSocket);
		return EXIT_ERROR;
	}

	// Free the used resources.
	free(sendbuf);

	// Free the response headers.
	if(httpResponse.headers != NULL) {
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
char *createHttpResponseAsString(HttpResponse *httpResponse,
							     HttpRequest *httpRequest,
							     int *size) {
	char *headbuf = NULL;
	int responseLen;

	responseLen = calculateHttpRespnseLength(httpResponse);
	
	headbuf = (char *) malloc((responseLen + 1) * sizeof(char));

	// Create the status line of the response.
	createHttpResponseStatusLine(httpResponse, httpRequest, headbuf, size);
	// If no body exists for the response, do not create unnecessary headers.
	if (httpResponse->body != NULL) {
		sprintf(headbuf + *size, "%s: %s;%s=%s\r\n%s: %d\r\n%s\r\n", 
			CONTENT_TYPE, httpResponse->headers->contentType, CHARSET, httpResponse->headers->charset, 
			CONTENT_LENGTH, httpResponse->headers->contentLength, CONNECTION_CLOSE);

		// Copy the contents of the message body to the char array.
		*size = strlen(headbuf);

		//sprintf(headbuf + *size, "%s", httpResponse->body);
		memcpy(headbuf + *size, httpResponse->body, httpResponse->headers->contentLength);
	} else {
		sprintf(headbuf + *size, "%s\r\n", CONNECTION_CLOSE);
	}

	*size = responseLen;

	return headbuf;
}

/*
Calculates the length of the HTTP response.
INPUT: The HttpResponse structure which contains the response message data.
OUTPUT: The length of the HTTP message.
*/
int calculateHttpRespnseLength(HttpResponse *httpResponse) {
	int totalLength, contentTypeLen, charsetLen, connectionClose, contentLengthLen, contentLengthTemp;
	totalLength = 0;
	contentTypeLen = 0;
	charsetLen = 0;
	contentLengthLen = 0;
	/*
	Calculating the length of the status line - add the length of each token and add one byte for each 
	of the two spaces and 2 bytes for the CRLF at the end of the status line.
	*/
	totalLength += strlen(httpResponse->status->statusCode) + 
		strlen(httpResponse->status->reasonPhrase) + strlen(httpResponse->httpVersion) + 2 + strlen(CRLF);

	// When no body exists, the headers that describe the body are considered.
	if (httpResponse->body != NULL) {
		/*
		The length of the Content-Type header is the length of the header's filed name, the length of the 
		field value and two characters for the colon and space --> field-name: field-value
		*/
		contentTypeLen = strlen(CONTENT_TYPE) + strlen(httpResponse->headers->contentType) + 2;

		/*
		The idea here resembles the one from above. The length includes the semi-colon before the charset
		directive and the equals sign after it and the actual value for the charset. The CRLF at the end of 
		the line must also be accounted	for.
		*/
		charsetLen = strlen(CHARSET) + strlen(httpResponse->headers->charset) + 2 + strlen(CRLF);

		/*
		The content length field is a bit different - the number needs to be treated as a string and the
		length of that string needs to be acquired.
		*/
		contentLengthLen = 0;
		contentLengthTemp = httpResponse->headers->contentLength;
		while(contentLengthTemp != 0) {
			contentLengthTemp /= 10;
			contentLengthLen++;
		}

		/*
		Add the length of the field name and add two bytes for the colon and space characters
		and the length of the CRLF delimiter.
		*/
		contentLengthLen += strlen(CONTENT_LENGTH) + 2 + strlen(CRLF);
		
		// Add the length of the body at the end.
		totalLength += httpResponse->headers->contentLength;
	}

	// Get the lengh of the entrie header.
	connectionClose = strlen(CONNECTION_CLOSE);

	//When the header section is compelted, a CRLF delimiter must be placed at the end.
	totalLength += contentTypeLen + charsetLen + contentLengthLen + 
		connectionClose + strlen(CRLF);

	return totalLength;
}

/*
This function converts the status line of the HttpResponse structure into a string.

INPUT: 'httpResponse' is the a pointer to the structurw which will contain the HTTP response data;
'httpRequest' is a pointer to the structure which contains the HTTP request; 'headbuf'is the buffer which
will contain the parsed header information; 'length' will conatin the new length of the buffer
after the parsing is complete.
*/
void createHttpResponseStatusLine(HttpResponse *httpResponse, 
								  HttpRequest *httpRequest, 
								  char *headbuf,
								  int *length) {
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
	headbuf[headbuflen] = LF;
	
	*length = headbuflen + 1;
}

/*
This function creates an HttpResponse for an unsupported HTTP method.

INPUT: 'httpRequest' is a pointer to an HttpRequest structure; 'httpResponse' is a
pointer to an HttpResponse structure whose fields will be created accordingly.
*/
void provideUnknown(HttpRequest *httpRequest,
				 HttpResponse *httpResponse) {
	httpResponse->httpVersion = httpRequest->requestLine->httpVersion;
	httpResponse->status = &StatusCodesTable.NotImplemented;
}

/*
This function creates an HttpResponse for an the HTTP HEAD method.

INPUT: 'httpRequest' is a pointer to an HttpRequest structure; 'httpResponse' is a
pointer to an HttpResponse structure whose fields will be created accordingly.
*/
void provideHead(HttpRequest *httpRequest,
				 HttpResponse *httpResponse) {

	FILE *fd = NULL;

	httpResponse->httpVersion = httpRequest->requestLine->httpVersion;
	
	// Open the file for reading.
	fd = fopen(httpRequest->requestLine->requestURI, "rb");

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
		httpResponse->headers->contentType = getResourceContentType(httpRequest->requestLine->requestURI);
		httpResponse->headers->charset = httpRequest->headers->charset  == NULL ? (char*)"UTF-8" : httpRequest->headers->charset;

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
void provideGet(HttpRequest *httpRequest,
				 HttpResponse *httpResponse) {
	FILE *fd = NULL;
	// The GET method uses the same status line and headers created when responding to the HEAD method.
	provideHead(httpRequest, httpResponse);
	// Acquire the file's contents and add them to the response.
	if(httpResponse->status->statusCode != StatusCodesTable.NotFound.statusCode) {
		fd = fopen(httpRequest->requestLine->requestURI, "rb");
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
void providePost(HttpRequest *httpRequest,
				 HttpResponse *httpResponse) {
	
	FILE *fd = NULL;
	
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

INPUT: 'httpResponse' is a pointer to an HttpResponse structure whose fields will
be created accordingly.
*/
void provideOptions(HttpResponse *httpResponse) {
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
