#ifndef HTTP_COMMONS_H
#define HTTP_COMMONS_H

#define SP ' '
#define CRLF "\r\n"
#define CR '\r'
#define LF '\n'

#define DEFAULT_BUFLEN 1024 // 1KB
#define EXIT_ERROR 1
#define EXIT_OK 0

#define EMPTY_STRING ""

enum HTTPMETHOD {
	HEAD, GET, POST, OPTIONS, UNKNOWN_METHOD
};

#endif