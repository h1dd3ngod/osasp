#include <stdio.h>
#include <sys/stat.h>
#include <stdlib.h>

#define EXIT_ON_ERROR -1

int main(int argc, char *argv[])
{

    if (argc != 3)
    {
        fprintf(stderr, "Wrong composition of arguments\nProper format: %s <Source file> <Destination file>\n", argv[0]);
        return EXIT_ON_ERROR;
    }

    FILE *srcFile = fopen(argv[1], "r");

    if (srcFile == NULL)
    {
        fprintf(stderr, "The source file couldn't have been opened\n");
        return EXIT_ON_ERROR;
    }

    FILE *destFile = fopen(argv[2], "w");

    if (destFile == NULL)
    {
        fprintf(stderr, "The destination file couldn't have been opened\n");
        return EXIT_ON_ERROR;
    }

    char nextChar;
    while ((nextChar = getc(srcFile)) != EOF)
    {
        putc(nextChar, destFile);
    }

    struct stat sourceStats;
    stat(argv[1], &sourceStats);
    chmod(argv[2], sourceStats.st_mode);

    if (fclose(srcFile))
    {
        fprintf(stderr, "The file %s couldn't be written to\n", argv[1]);
        return EXIT_ON_ERROR;
    }
    if (fclose(destFile))
    {
        fprintf(stderr, "The file %s couldn't be written to\n", argv[2]);
        return EXIT_ON_ERROR;
    }
    printf("Operation was successful.\n");
    return 0;
}