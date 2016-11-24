#ifndef FILE_UTILS_H
#define FILE_UTILS_H

#include <Windows.h>
#include <stdio.h>

int fileExists(const char *filename);

char *provideFileContents(FILE *fp);

#endif
