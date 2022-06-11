#include <stdio.h>
#include <semaphore.h>
#include <malloc.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <sys/time.h>
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>

#define CHILD_COUNT 8

typedef struct Node
{
    int val;
    struct Node *cn[CHILD_COUNT];
    int signal;  // -1 - no signal
    int signal2; // -1 - no signal
    sigset_t *sigset;
} Node;

char *moduleName;

Node *currNode = NULL;
int SIGUSR1_recieved = 0;
int SIGUSR2_received = 0;
int SIGUSR1_sent = 0;
int SIGUSR2_sent = 0;

long long getCurrTime()
{
    struct timeval tv;

    if (gettimeofday(&tv, NULL) == -1)
    {
        // perror("Can not get current time");
        fprintf(stderr, "%s: %s", moduleName, "Can not get current time");
        return -1;
    }

    return tv.tv_sec * 1000000 + tv.tv_usec;
}

void sendToAll(int signal);
int retrievePid(int val);
int getValByPid(int pid)
{
    for (int i = 0; i < CHILD_COUNT; ++i)
    {
        if (retrievePid(i) == pid)
        {
            return i;
        }
    }
    return -1;
};

// char* getNameOfSignal(int signal) {
//     if (signal == SIGUSR1) return "SIGUSR1";
//     else if (signal == SIGUSR2) return "SIGUSR2";
// }

void printSignalSent(int fromPid, int fromVal, int toPid, int signal, int toVal)
{
    char sigStr[8];
    if (signal == SIGUSR1)
    {
        strcpy(sigStr, "SIGUSR1");
    }
    else if (signal == SIGUSR2)
    {
        strcpy(sigStr, "SIGUSR2");
    }
    printf("Signal sent from %d(value = %d) to %d(value = %d), signal: %s (at %lld)\n", fromPid, fromVal, toPid, toVal, sigStr, getCurrTime());
}
void signalHandler(int sig, siginfo_t *siginfo, void *code)
{
    char sigStr[8];
    if (sig == SIGUSR1)
    {
        strcpy(sigStr, "SIGUSR1");
        SIGUSR1_recieved++;
    }
    else if (sig == SIGUSR2)
    {
        strcpy(sigStr, "SIGUSR2");
        SIGUSR2_received++;
    }
    printf("%d(%d) received %s from %d(%d) (at %lld)\n", getpid(), currNode->val, sigStr, siginfo->si_pid,
           getValByPid(siginfo->si_pid), getCurrTime());

    if (sig == SIGTERM)
    {
        printf("{%lld}  %d(%d) got TERMINATED after sent: USR1: %d; USR2: %d;\n",
               getCurrTime(),
               getpid(),
               currNode->val,
               SIGUSR1_sent,
               SIGUSR2_sent);
        exit(0);
    }
    if (currNode->val == 1 && SIGUSR1_recieved == 101)
    {
        sendToAll(SIGTERM);
        while (wait(NULL) != -1)
            ;
        exit(0);
    }

    if (currNode->val == 1)
    {
        SIGUSR2_sent++;
        sendToAll(SIGUSR2);
    }
    else
    {
        for (int i = 0; i < CHILD_COUNT; ++i)
        {
            if (currNode->cn[i] != NULL)
            {

                int toVal = currNode->cn[i]->val;
                int toPid = retrievePid(toVal);
                printSignalSent(getpid(), currNode->val, toPid, SIGUSR1, toVal);
                SIGUSR1_sent++;
                if (kill(toPid, SIGUSR1))
                    perror("Can't kill");
            }
        }
    }
}

void sendToAll(int signal)
{
    __pid_t pgid = getpgid(getpid());
    printSignalSent(getpid(), currNode->val, (-1) * pgid, signal, -1);
    if (kill((-1) * pgid, signal))
        perror("Can't send signal");
}

Node *newNode(int val, Node *parent, int signal, int signal2)
{
    Node *n = (Node *)malloc(sizeof(Node));
    n->signal = signal;
    n->signal2 = signal2;
    n->val = val;
    n->sigset = (sigset_t *)malloc(sizeof(sigset_t));
    for (int i = 0; i < CHILD_COUNT; ++i)
    {
        n->cn[i] = NULL;
    }
    if (parent != NULL)
    {
        char nodeAdded = 0;
        for (int i = 0; i < CHILD_COUNT; ++i)
        {
            if (parent->cn[i] == NULL)
            {
                parent->cn[i] = n;
                nodeAdded = 1;
                break;
            }
        }
        if (!nodeAdded)
        {
            perror("Error: node not added\n");
        }
    }
    return n;
}

void savePid(int val, int pid)
{
    key_t key = ftok("shmfile", 65);
    int shmid = shmget(key, 1024, 0666 | IPC_CREAT);
    int *str = (int *)shmat(shmid, (void *)0, 0);

    str[val] = pid;

    shmdt(str);
}

int retrievePid(int val)
{
    key_t key = ftok("shmfile", 65);
    int shmid = shmget(key, 1024, 0666 | IPC_CREAT);
    int *str = (int *)shmat(shmid, (void *)0, 0);

    int ret = str[val];

    shmdt(str);
    return ret;
}

void initTree(Node *root)
{
    Node *n1 = newNode(1, root, SIGUSR1, 0);
    Node *n2 = newNode(2, n1, SIGUSR2, SIGTERM);
    Node *n3 = newNode(3, n1, SIGUSR2, SIGTERM);
    Node *n4 = newNode(4, n1, SIGUSR2, SIGTERM);
    newNode(5, n1, SIGUSR2, SIGTERM);

    newNode(6, n2, SIGUSR1, SIGTERM);

    newNode(7, n3, SIGUSR1, SIGTERM);
    Node *n8 = newNode(8, n4, SIGUSR1, SIGTERM);
    n8->cn[0] = n1;
}

void establishSigHandler(Node *n)
{
    if (n->signal == -1)
        return;

    if (sigfillset(n->sigset))
    {
        perror("Error: filling set");
    }
    if (n->signal != 0)
    {
        if (sigdelset(n->sigset, n->signal))
        {
            perror("Error: deleting set");
        }
    }
    if (n->signal2 != 0)
    {
        if (sigdelset(n->sigset, n->signal2))
        {
            perror("Error: deleting set");
        }
    }
    if (sigprocmask(SIG_SETMASK, n->sigset, NULL) < 0)
    {
        perror("Error: setting signal mask");
    }

    struct sigaction act;
    memset(&act, 0, sizeof(act));
    act.sa_sigaction = signalHandler;
    act.sa_mask = *(n->sigset);
    act.sa_flags = SA_SIGINFO;
    if (n->signal && sigaction(n->signal, &act, 0))
    {
        perror("Error: registering signal handler");
    }

    if (n->signal2 && sigaction(n->signal2, &act, 0))
    {
        perror("Error: registering signal handler");
    }
}

pid_t forkProcess(Node *n)
{
    pid_t pid = fork();
    switch (pid)
    {
    case -1:
        perror("Error: fork failed\n");
        break;
    case 0:
        printf("Forked process %d(%d) from %d(%d)\n", getpid(), n->val, getppid(), currNode->val);
        currNode = n;
        savePid(currNode->val, getpid());
        establishSigHandler(currNode);

        return 0;
    default:
        return pid;
    }
}

int created[CHILD_COUNT] = {};

// #pragma ide diagnostic ignored "misc-no-recursion"
void createProcessTree(Node *root)
{
    created[root->val] = 1;
    currNode = root;
    for (int i = 0; i < CHILD_COUNT; ++i)
    {
        struct Node *nextNode = currNode->cn[i];
        if (nextNode != NULL && !created[nextNode->val])
        {
            char semName[255] = "/child_register_handler";
            size_t strlen1 = strlen(semName);
            semName[strlen1] = '0' + nextNode->val;
            semName[strlen1 + 1] = '\0';
            sem_t *childRegisterHandler = sem_open(semName, O_CREAT, 0777, 0);
            if (childRegisterHandler == SEM_FAILED)
            {
                perror("Couldn't create a semaphore\n");
                // exit(EXIT_FAILURE);
            }
            pid_t pid = forkProcess(nextNode);
            if (pid == 0)
            {
                // for child process
                createProcessTree(currNode);
                sem_post(childRegisterHandler);
                if (currNode->val == 1)
                {
                    sendToAll(SIGUSR2);
                }
                while (1)
                {
                    sleep(20000);
                }
                printf("Exiting %d\n", currNode->val);
                exit(0);
            }
            else
            {
                // for parent process //
                sem_wait(childRegisterHandler);
                printf("Process tree for %d created\n", nextNode->val);
                sem_destroy(childRegisterHandler);
                continue;
            }
        }
    }
}

int main(int argc, char *argv[])
{
    // strcpy(moduleName, argv[0]);
    Node *root = newNode(0, NULL, 0, 0);
    establishSigHandler(root);
    initTree(root);
    createProcessTree(root);
    while (wait(NULL) != -1)
        ;
    return 0;
}