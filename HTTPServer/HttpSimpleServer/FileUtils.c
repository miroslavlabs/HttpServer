#include "FileUtils.h"
#include <stdio.h>

/*
This function reads the contents of a file and returns a pointer to
a char array where the contents of the file are copied.

INPUT: 'fp' is a pointer to the FILE strucutre of an opend file. The file
must have been opened for reading.

OUTPUT: A pointer to a char array where the file contents were copied.
*/
char* provideFileContents(__in FILE *fp) {	
	long file_size;
	char *buffer;

	/*
	First, the file size needs to be discovered. To do that, seek the end of the file
	and then we rewind the file pointer to read from the begging of the file.
	*/
	fseek(fp , 0L , SEEK_END);
	file_size = ftell(fp);
	rewind(fp);

	// Allocate memory to store the entire file's contents.
	buffer = (char*)calloc(file_size + 1, sizeof(char));
	if(!buffer) {
		return NULL;
	}

	// Copy the file into the bufffer.
	fread(buffer , file_size, 1 , fp);

	buffer[file_size] = '\0';

	return buffer;
}

/*
This function verifies that a file with the provided name exists in the
file system.

INPUT: 'filename' contains the name of the file.

OUTPUT: TRUE if the file exists and FALSE otherwise.
*/
BOOL fileExists(__in const char *filename)
{
   FILE *fp = fopen (filename, "r");
   if (fp != NULL) fclose (fp);
   return (fp != NULL);
}