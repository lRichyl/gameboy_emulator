#include <stdlib.h>
#include <stdio.h>

#include "file_handling.h"

char* text_file_to_string(const char* file_name){
	FILE *fp;
	long file_size;
	// char *buffer;

	fopen_s(&fp, file_name , "rb" );
	if( !fp ){
		printf("File could not be opened\n" );
        return NULL;
	}

	fseek(fp, 0L, SEEK_END);
    file_size = ftell(fp);
    rewind(fp);

    // Allocate memory for buffer
    char *buffer = (char *)malloc(file_size + 1);
    if (!buffer) {
        fclose(fp);
        printf("Error: Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }

    // Read file contents into buffer
    if (fread(buffer, file_size, 1, fp) != 1) {
        fclose(fp);
        free(buffer);
        printf("Error: Failed to read entire file %s\n", file_name);
        exit(EXIT_FAILURE);
    }

    // Null-terminate the buffer
    buffer[file_size] = '\0';

    // Close file and return buffer
    fclose(fp);
    return buffer;
}

u8* load_binary_file(const char* file_name, u32 *file_size){
    FILE *fp;

    fopen_s(&fp, file_name , "rb" );
    if( !fp ){
        printf("File could not be opened\n" );
        return NULL;
    }

    fseek(fp, 0L, SEEK_END);
    assert(file_size);
    *file_size = ftell(fp);
    rewind(fp);

    // Allocate memory for buffer
    u8 *buffer = (u8 *)malloc(*file_size);
    if (!buffer) {
        fclose(fp);
        printf("Error: Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }

    // Read file contents into buffer
    if (fread(buffer, *file_size, 1, fp) != 1) {
        fclose(fp);
        printf("Error: Failed to read entire file %s\n", file_name);
        exit(EXIT_FAILURE);
    }

    // Close file and return buffer
    fclose(fp);
    return buffer;
}

bool file_exists(const char * filename)
{
    FILE *file = NULL;
    fopen_s(&file, filename, "r");
    if (file)
    {
        fclose(file);
        return true;
    }

    return false;
}
