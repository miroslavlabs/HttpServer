#ifndef HTTP_COMMONS_H
#define HTTP_COMMONS_H

#define DEFAULT_BUFLEN 1024 // 1KB
#define EXIT_ERROR 1
#define EXIT_OK 0

// Literals

#define EMPTY_STRING ""
#define DOT '.'
#define SP ' '
#define CRLF "\r\n"
#define CR '\r'
#define LF '\n'

#define CONTENT_LENGTH "Content-Length"
#define CONTENT_TYPE "Content-Type"
#define CONNECTION_CLOSE "Connection: close\r\n"
#define CHARSET "charset"

// File extensions

#define TEXT_EXT "txt"
#define HTML_EXT "html"
#define JPEG_EXT "jpeg"
#define JPG_EXT "jpg"
#define PNG_EXT "png"
#define GIF_EXT "gif"

// MIME types

#define PLAIN_TEXT_MIME_TYPE "text/plain"
#define HTML_TEXT_MIME_TYPE "text/html"
#define JPEG_IMAGE_MIME_TYPE "image/jpeg"
#define PNG_IMAGE_MIME_TYPE "image/png"
#define GIF_IMAGE_MIME_TYPE "image/gif"

/*
The enumaration contains all of the supported HTTP methods.
*/
enum HTTPMETHOD {
	HEAD, GET, POST, OPTIONS, UNKNOWN_METHOD
};

char *acquireMime(char *fileExtension);

char *getResourceContentType(char *resource);

#endif
