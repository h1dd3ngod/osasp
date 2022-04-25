#include <stdio.h>
#include <errno.h>
#include <stdlib.h>

#define EXIT_ON_ERROR -1

int checkIfValidInt(const char* str, int* toInt) {
    char* errChar;
    *toInt = (int)strtol(str, &errChar, 10);
    if (errno != 0 || *toInt < 0)
    {
        fprintf(stderr, "Wrong value of second parameter %s\n", str);
        return EXIT_ON_ERROR;
    }
    if (*errChar != '\0' || errChar == str) {
        fprintf(stderr, "wrong char encountered\n");
        return EXIT_ON_ERROR;
    }
    return EXIT_SUCCESS;
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        fprintf(stderr, "Unsuitable parameter composition\nProper format: %s <filename> <numberOfLinesPerGroup>\n", argv[0]);
        return 0;
    }
    int linesPerGroup;
    if (checkIfValidInt(argv[2], &linesPerGroup)) return EXIT_ON_ERROR;

    FILE *fileToRead = fopen(argv[1], "r");
    if (fileToRead == NULL)
    {
        fprintf(stderr, "The file couldn't have been opened. Terminated...\n");
        return EXIT_ON_ERROR;
    }
    char nextChar;
    int linesPrinted = 0;
    while ((nextChar = getc(fileToRead)) != EOF)
    {
        if (nextChar == '\n') linesPrinted++;
        if (linesPrinted == linesPerGroup && linesPerGroup != 0)
        {
            getc(stdin);
            linesPrinted = 0;
        }
        putc(nextChar, stdout);
    }
    putc('\n', stdout);
    if (fclose(fileToRead))
    {
        fprintf(stderr, "The file %s couldn't have been closed\n", argv[1]);
        return EXIT_ON_ERROR;
    }

    return 0;
}