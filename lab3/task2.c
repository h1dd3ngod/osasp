#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <stdlib.h>
#define EXIT_ERROR -1
#define BUFFER 19

void printProcInfo()
{
    printf("Current process pid: %d, Parent process pid: %d, ", getpid(), getppid());
    struct timespec tc;
    timespec_get(&tc, TIME_UTC);
    char buffer[BUFFER];
    strftime(buffer, BUFFER, "%X", gmtime(&tc.tv_sec));
    printf("System Time is: %s.%3ld (UTC)\n", buffer, tc.tv_nsec);
}

int errorCreatingProcess() {
    fprintf(stderr, "Couldn't have created appropriate processes!\n");
    return EXIT_ERROR;
}

int main()
{
    pid_t subProc = fork();
    if (errno) return errorCreatingProcess();
    if (subProc > 0) {
        subProc = fork();
        if (errno) return errorCreatingProcess();
    }
    printProcInfo();
    if (subProc)
        system("ps -x ");
    else
        return 0;
    return 0;
}