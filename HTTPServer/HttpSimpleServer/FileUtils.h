#ifndef FILE_UTILS_H
#define FILE_UTILS_H

#include <Windows.h>
#include <stdio.h>

int fileExists(__in const char *filename);

char *provideFileContents(__in FILE *fp);

#endif