#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

long long from_size, to_size, from_date, to_date;
char src[1024];
int srcLen;
FILE *resFile;

int isNotAnotherDirectory(const char *dest)
{
    if (strcmp(dest, ".") == 0 || strcmp(dest, "..") == 0)
    {
        return 0;
    }
    return EXIT_FAILURE;
}

int isDirectory(const struct stat *st)
{
    if (S_ISDIR(st->st_mode))
    {
        return EXIT_FAILURE;
    }
    return 0;
}

void writeAndPrintFile(const struct stat *st)
{
    if (from_size <= st->st_size && st->st_size <= to_size && from_date <= st->st_ctim.tv_sec && st->st_ctim.tv_sec <= to_date)
        printf("%s    %ld  %ld\n", src, st->st_size, st->st_ctim.tv_sec);
    fprintf(resFile, "%s    %ld  %ld\n", src, st->st_size, st->st_ctim.tv_sec);
}

void enterSelf()
{
    struct stat st;
    if (stat(src, &st))
    {
        fprintf(stderr, "File/src (%s) can't be read.\n", src);
        return;
    }
    if (isDirectory(&st))
    {
        DIR *dirPtr = opendir(src);
        if (!dirPtr)
        {
            fprintf(stderr, "Can't open directory %s!\n", src);
            return;
        }

        struct dirent *dirOrFile;
        while ((dirOrFile = readdir(dirPtr)))
        {
            char *destName = dirOrFile->d_name;
            int destNameLen = strlen(destName);

            if (isNotAnotherDirectory(destName))
            {
                int oldDirLen = srcLen;

                strcat(src, "/");
                strcat(src, destName);
                srcLen += 1 + destNameLen;

                enterSelf();

                srcLen -= 1 + destNameLen;
                src[oldDirLen] = '\0';
            }
        }

        if (closedir(dirPtr))
        {
            fprintf(stderr, "Can't close directory %s!\n", src);
            return;
        }
    }
    else
    {
        writeAndPrintFile(&st);
    }
}

int argStrToInt(char *name, char *from, long long *to)
{
    *to = strtol(from, NULL, 10);
    if (errno != 0 || *to < 0)
    {
        fprintf(stderr, "The parameter %s must be a positive integer.\n", name);
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char *argv[])
{
    if (argc != 7)
    {
        fprintf(stderr, "Wrong argument composition.\nProper format: %s <directory_name> <output_file> <from_size> <to_size> <from_date> <to_date>", argv[0]);
        return EXIT_FAILURE;
    }

    resFile = fopen(argv[2], "w");
    if (resFile == NULL)
    {
        fprintf(stderr, "Error when creating output file");
        return EXIT_FAILURE;
    }

    argStrToInt("from_size", argv[3], &from_size);
    argStrToInt("to_size", argv[4], &to_size);
    argStrToInt("from_date", argv[5], &from_date);
    argStrToInt("to_date", argv[6], &to_date);

    strcpy(src, argv[1]);
    srcLen = strlen(src);
    enterSelf();

    if (fclose(resFile))
    {
        fprintf(stderr, "File %s can't be closed", argv[2]);
        return EXIT_FAILURE;
    }
    return 0;
}