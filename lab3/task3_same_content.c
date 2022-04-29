#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <wait.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/wait.h>

// Secondary functions
void abruptError(const char *str)
{
    perror(str);
    exit(EXIT_FAILURE);
}

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

int checkIfValidInt(const char *str, int *toInt)
{
    char *errChar;
    *toInt = (int)strtol(str, &errChar, 10);
    if (errno != 0 || *toInt < 0)
    {
        fprintf(stderr, "Wrong value of third parameter %s\n", str);
        return EXIT_FAILURE;
    }
    if (*errChar != '\0' || errChar == str)
    {
        fprintf(stderr, "Wrong char encountered\n");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

char srcPath[PATH_MAX];
int srcLen;
char destDir[PATH_MAX];
int destLen;
int N;
int procCounter;

void printProcInfo(pid_t pid, const char *src, const char *dest, long size, const char *compared)
{
    printf("Current active process number: %d, Process id: %d, Src file name: %s, Dest file name: %s, Size: %ld, Result: %s\n", procCounter, pid, src, dest, size, compared);
    // printf("---------------------------------------------------------------------------------------------------------------------------------\n");
}

// Main task core functions
void compareFilesAndPrint(char *srcName, char *destName)
{
    FILE *srcDirFile = fopen(srcName, "r");

    if (srcDirFile == NULL)
    {
        fprintf(stderr, "File %s can't be opened\n", srcName);
        return;
    }

    FILE *destDirFile = fopen(destName, "r");

    if (destDirFile == NULL)
    {
        fprintf(stderr, "File %s can't be opened\n", destName);
        return;
    }

    struct stat status;
    if (stat(srcName, &status))
    {
        fprintf(stderr, "Couldn't load the status of a source file");
        return;
    }
    int srcFileSize = status.st_size;
    if (stat(destName, &status))
    {
        fprintf(stderr, "Couldn't load the status of a destination file");
        return;
    }
    int destFileSize = status.st_size;

    // comparing files
    char comparisonRes[50];
    if (srcFileSize > destFileSize)
        printProcInfo(getpid(), srcName, destName, srcFileSize, "Source file is larger");
    else if (destFileSize > srcFileSize)
        printProcInfo(getpid(), srcName, destName, destFileSize, "Source file is smaller");
    else
    {
        char nextCharSrc, nextCharDest;
        int differentContent = 0;
        while ((nextCharSrc = fgetc(srcDirFile)) != EOF && (nextCharDest = fgetc(destDirFile)) != EOF && !differentContent)
            if (nextCharSrc != nextCharDest)
                differentContent = 1;
        if (differentContent)
            printProcInfo(getpid(), srcName, destName, destFileSize, "Files are the same size");
        else
            printProcInfo(getpid(), srcName, destName, destFileSize, "Files are EQUAL");
    }

    if (fclose(srcDirFile))
    {
        fprintf(stderr, "The file %s couldn't have been closed\n", srcName);
        return;
    }
    if (fclose(destDirFile))
    {
        fprintf(stderr, "The file %s couldn't have been closed\n", destName);
        return;
    }
}

void createThreadCompareFiles(char *srcName, char *destName)
{
    if (procCounter >= N)
    {
        if (wait(NULL) == -1)
            abruptError("Can't wait for child process");
    }
    else
        procCounter++;
    pid_t childProc = fork();
    if (childProc < 0)
        abruptError("Couldn't create a child process");
    if (childProc == 0)
    {
        compareFilesAndPrint(srcName, destName);
        exit(EXIT_SUCCESS);
    }
}

void enterSelf()
{
    struct stat st;
    if (stat(srcPath, &st))
    {
        fprintf(stderr, "File/src (%s) can't be read.\n", srcPath);
        return;
    }
    if (isDirectory(&st))
    {
        DIR *dirPtr = opendir(srcPath);
        if (!dirPtr)
        {
            fprintf(stderr, "Can't open directory %s!\n", srcPath);
            return;
        }

        struct dirent *d;
        while ((d = readdir(dirPtr)))
        {
            char *destName = d->d_name;
            int destNameLen = strlen(destName);

            if (isNotAnotherDirectory(destName))
            {
                int oldDirLen = srcLen;
                int oldDestLen = destLen;

                strcat(srcPath, "/");
                strcat(srcPath, destName);
                srcLen += 1 + destNameLen;

                enterSelf();

                srcLen -= 1 + destNameLen;
                srcPath[oldDirLen] = '\0';
            }
        }

        if (closedir(dirPtr))
        {
            fprintf(stderr, "Can't close directory %s!\n", srcPath);
            return;
        }
    }
    else
    {
        DIR *destDirPtr = opendir(destDir);
        if (!destDirPtr)
        {
            fprintf(stderr, "Can't open the destined for directory %s!\n", destDir);
            return;
        }
        struct dirent *destFile;
        while ((destFile = readdir(destDirPtr)))
        {
            char currFileName[PATH_MAX];
            strcpy(currFileName, destDir);
            strcat(currFileName, "/");
            strcat(currFileName, destFile->d_name);

            struct stat fileState;
            if (stat(currFileName, &fileState))
            {
                fprintf(stderr, "Dest file (%s) can't be read.\n", destDir);
                return;
            }

            if (!isDirectory(&fileState))
                createThreadCompareFiles(srcPath, currFileName);
        }
        if (closedir(destDirPtr))
        {
            fprintf(stderr, "Can't close directory %s!\n", destDir);
            return;
        }
    }
}

int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        fprintf(stderr, "You have entered unsuitable arguments\n");
        fprintf(stderr, "Proper format: <Source folder> <Destination folder> <N(Max count of processes)>\n");
        return EXIT_FAILURE;
    }
    strcpy(srcPath, argv[1]);
    strcpy(destDir, argv[2]);
    if (checkIfValidInt(argv[3], &N))
        return EXIT_FAILURE;
    srcLen = strlen(srcPath);
    destLen = strlen(destDir);

    enterSelf();

    while (wait(NULL) != -1);
    // upon completion of all concurrent processes
    return EXIT_SUCCESS;
}