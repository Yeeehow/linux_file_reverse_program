#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define BUFFER_SIZE 1024

int main() {
    char buffer[BUFFER_SIZE];
    ssize_t bytesRead;
    size_t totalSize = 0;
    char* inputData = NULL;

    while ((bytesRead = read(STDIN_FILENO, buffer, BUFFER_SIZE)) > 0) {
        char* newData = realloc(inputData, totalSize + bytesRead);
        if (!newData) {
            perror("Memory allocation failed");
            free(inputData);
            return EXIT_FAILURE;
        }
        inputData = newData;
        memcpy(inputData + totalSize, buffer, bytesRead);
        totalSize += bytesRead;
    }

    if (bytesRead < 0) {
        perror("Error reading from standard input");
        free(inputData);
        return EXIT_FAILURE;
    }

    //fprintf(stderr, "Reverser Debug: Received input (%ld bytes): %.*s\n", totalSize, (int)totalSize, inputData);

    for (ssize_t i = totalSize - 1; i >= 0; i--) {
        if (write(STDOUT_FILENO, &inputData[i], 1) < 0) {
            perror("Error writing to standard output");
            free(inputData);
            return EXIT_FAILURE;
        }
    }
    fflush(stdout);
    if (totalSize > 0 && inputData[totalSize - 1] != '\n') {
        ssize_t bytesWritten = write(STDOUT_FILENO, "\n", 1);
        if (bytesWritten < 0) {
            perror("Error writing newline to standard output");
            free(inputData);
            return EXIT_FAILURE;
        }
    }

    free(inputData);
    fflush(stdout);
    return EXIT_SUCCESS;
}
