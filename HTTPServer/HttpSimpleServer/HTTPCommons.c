#include "HTTPCommons.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/*
Acquire the MIME type of the file based on the file extension of the resource.
INPUT: The file extension.
OUTPUT: The corresponsing MIME type.
*/
char *acquireMime(char *fileExtension) {
	if(strcmp(fileExtension, TEXT_EXT) == 0) {
		return PLAIN_TEXT_MIME_TYPE;
	} else if(strcmp(fileExtension, HTML_EXT) == 0) {
		return HTML_TEXT_MIME_TYPE;
	} else if(strcmp(fileExtension, JPEG_EXT) == 0) {
		return JPEG_IMAGE_MIME_TYPE;
	} else if(strcmp(fileExtension, JPG_EXT) == 0) {
		return JPEG_IMAGE_MIME_TYPE;
	} else if(strcmp(fileExtension, PNG_EXT) == 0) {
		return PNG_IMAGE_MIME_TYPE;
	}else if(strcmp(fileExtension, GIF_EXT) == 0) {
		return GIF_IMAGE_MIME_TYPE;
	}
	
	return NULL;
}

/*
Acquire the content type of the queried resource.
INPUT: The file path of the resource.
OUTPUT: The corresponsing content type.
*/
char *getResourceContentType(char *resource) {
	char *fileExtension = NULL;
	int resourceNameLength, currentPos, extensionLength;
	
	resourceNameLength = strlen(resource);
	// Search from the end of the string forwards.
	currentPos = resourceNameLength - 1;
	// Search for the dot - it delimits the file name from the extension.
	while(*(resource + currentPos) != DOT) {
		currentPos--;
	}

	// Calcualte the length of the extension and acquire just the extension.
	extensionLength = resourceNameLength - currentPos;
	fileExtension = (char *) malloc((extensionLength + 1) * sizeof(char));
	memcpy(fileExtension, resource + currentPos + 1, extensionLength);

	return acquireMime(fileExtension);
}
