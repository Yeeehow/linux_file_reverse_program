#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>

#define MAX_LINES 10
#define BUFFER_SIZE 1024

void error_exit(const char* message) {
    perror(message);
    exit(EXIT_FAILURE);
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        //fprintf(stderr, "Usage: %s <input.usp> <output.txt>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char* inputFile = argv[1];
    const char* outputFile = argv[2];

    int usp_fd = open(inputFile, O_RDONLY);
    if (usp_fd < 0) error_exit("Failed to open input file");

    char* lines[MAX_LINES];
    int lineCount = 0;
    char buffer[BUFFER_SIZE];
    char reversedLine[BUFFER_SIZE];
    ssize_t bytesRead;
    size_t bufferIndex = 0;

    while ((bytesRead = read(usp_fd, buffer + bufferIndex, BUFFER_SIZE - bufferIndex - 1)) > 0) {
        buffer[bufferIndex + bytesRead] = '\0';
        char* start = buffer;
        char* newline;

        while ((newline = strchr(start, '\n')) != NULL) {
            *newline = '\0';
            lines[lineCount] = strdup(start);
            if (!lines[lineCount]) error_exit("Memory allocation failed");
            //fprintf(stderr, "Debug: Line %d read: %s\n", lineCount, lines[lineCount]);
            lineCount++;
            if (lineCount >= MAX_LINES) {
                //fprintf(stderr, "Error: Too many lines in input file\n");
                exit(EXIT_FAILURE);
            }
            start = newline + 1;
        }

        bufferIndex = strlen(start);
        memmove(buffer, start, bufferIndex);
    }
    if (bytesRead < 0) error_exit("Error reading input file");
    if (bufferIndex > 0) {
        lines[lineCount] = strdup(buffer);
        if (!lines[lineCount]) error_exit("Memory allocation failed");
        //fprintf(stderr, "Debug: Line %d read (last): %s\n", lineCount, lines[lineCount]);
        lineCount++;
    }

    close(usp_fd);

    int pipes[MAX_LINES][2];
    pid_t pids[MAX_LINES];

    for (int i = 0; i < lineCount; i++) {
        if (pipe(pipes[i]) < 0) {
            perror("Pipe creation failed");
            //fprintf(stderr, "Pipe creation failed at index %d\n", i);
            exit(EXIT_FAILURE);
        }

        //fprintf(stderr, "Debug: Pipe created for line %d (read: %d, write: %d)\n", i, pipes[i][0], pipes[i][1]);

        pids[i] = fork();
        if (pids[i] < 0) error_exit("Fork failed");

        if (pids[i] == 0) {

            if (dup2(pipes[i][0], STDIN_FILENO) < 0) {
                perror("Child dup2 failed");
                exit(EXIT_FAILURE);
            }
            close(pipes[i][0]);
            //fprintf(stderr, "Debug: Child %d: Executing reverser\n", i);

            if (dup2(pipes[i][1], STDOUT_FILENO) < 0) {
                perror("Child dup#2 failed");
                exit(EXIT_FAILURE);
            }
            
            execlp("./reverser", "./reverser", NULL);
            perror("execlp failed");
            close(pipes[i][1]);
            exit(EXIT_FAILURE);
        }
        else {
            //fprintf(stderr, "Debug: Parent: Keeping read end of pipe[%d] open for reading\n", i);

            if (write(pipes[i][1], lines[i], strlen(lines[i])) < 0) {
                perror("Parent: Error writing to pipe");
                exit(EXIT_FAILURE);
            }
           //fprintf(stderr, "Debug: Parent: Wrote line %d (%s) to child\n", i, lines[i]);

            close(pipes[i][1]);
            //fprintf(stderr, "Debug: Parent: Closed write end of pipe[%d]\n", i);
        }
    }

    int output_fd = open(outputFile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (output_fd < 0) error_exit("Failed to open output file");

    for (int i = lineCount - 1; i >= 0; i--) {
        //fprintf(stderr, "Debug: Parent: Waiting for child[%d]\n", i);

        int status;
        if (waitpid(pids[i], &status, 0) == -1) error_exit("Error waiting for child process");

        if (!WIFEXITED(status)) {
            //fprintf(stderr, "Debug: Child %d did not exit successfully\n", i);
            continue;
        }

        //fprintf(stderr, "Debug: Parent: Reading from pipe[%d]\n", i);
        ssize_t lineLength = read(pipes[i][0], reversedLine, BUFFER_SIZE - 1);
        if (lineLength < 0) {
            perror("Error reading from pipe");
            //fprintf(stderr, "Failed to read from pipe[%d]\n", i);
            continue;
        }

        reversedLine[lineLength] = '\0';
        //fprintf(stderr, "Debug: Parent: Read %zd bytes from pipe[%d]: '%s'\n", lineLength, i, reversedLine);

        if (write(output_fd, reversedLine, lineLength) < 0 || write(output_fd, "\n", 1) < 0)
            error_exit("Error writing to output file");

        close(pipes[i][0]);
        //fprintf(stderr, "Debug: Parent: Closed read end of pipe[%d]\n", i);
    }

    close(output_fd);

    for (int i = 0; i < lineCount; i++) {
        free(lines[i]);
    }

    return 0;
}
