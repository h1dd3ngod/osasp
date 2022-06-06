#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>

#define MILISEC 1000
#define CHILD_COUNT 2
int messageCount = 1;

char *getCurrTime()
{
	struct timeval curTime;
	if (gettimeofday(&curTime, NULL) == -1)
	{
		perror("An error occured when recieving time: \n");
		return NULL;
	}
	int milisec = curTime.tv_usec / 1000;

	char buffer[9];
	if (strftime(buffer, 9, "%H:%M:%S", localtime(&curTime.tv_sec)) == 0)
	{
		fprintf(stderr, "Out of array bounds error");
		return NULL;
	}

	char *currentTime = malloc(sizeof(char) * 13);
	if (sprintf(currentTime, "%s:%03d", buffer, milisec) < 0)
	{
		fprintf(stderr, "Couldn't conver time");
		return NULL;
	}
	return currentTime;
}

void parentHandler(int signum, siginfo_t *currHandlerInfo, void *ucontext)
{
	char *time = getCurrTime();
	printf("PARENT: №%d of PID: %d PPID: %d %s recieved SIGUSR2 (from CHILD: %d)\n", messageCount++, getpid(), getppid(), time, currHandlerInfo->si_pid);
	usleep(100 * MILISEC);
	kill(0, SIGUSR1);	
	time = getCurrTime();
	printf("PARENT: №%d of PID: %d PPID: %d %s put SIGUSR1\n", messageCount++, getpid(), getppid(), time);
	free(time);
}

void childHandler(int signum, siginfo_t *currHandlerInfo, void *ucontext)
{
	char *time = getCurrTime();
	printf("№%d of PID: %d PPID: %d %s CHILD got SIGUSR1\n", messageCount++, getpid(), getppid(), time);
	kill(getppid(), SIGUSR2);
	time = getCurrTime();
	printf("№%d of PID: %d PPID: %d %s CHILD put SIGUSR2\n", messageCount++, getpid(), getppid(), time);
	free(time);
}

int main()
{
	struct sigaction sigact, childSigact;
	sigact.sa_flags = SA_SIGINFO;
	sigact.sa_sigaction = parentHandler;
	if (sigaction(SIGUSR2, &sigact, NULL) == -1)
	{
		perror("Failed to set handler");
	}
	if (signal(SIGUSR1, SIG_IGN) == SIG_ERR)
	{
		perror("Failed to init SIGUSR1");
	}

	pid_t childPid;
	for (int i = 0; i < CHILD_COUNT; i++)
	{
		childPid = fork();

		switch (childPid)
		{
		case 0:
			childSigact.sa_flags = SA_SIGINFO;
			childSigact.sa_sigaction = childHandler;
			if (sigaction(SIGUSR1, &childSigact, NULL) == -1)
			{
				perror("Failed to set handler");
			}
			while (1)
				sleep(2);
			break;

		case -1:
			perror("Error when creating a child process\n");
			exit(-1);
		default:
			break;
		}
	}

	kill(0, SIGUSR1);

	while (1)
		sleep(2);
}