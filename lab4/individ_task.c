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

Node *curNode = NULL;
int usr1_count_rec = 0;
int usr2_count_rec = 0;
int usr1_count_snd = 0;
int usr2_count_snd = 0;

long long getTime()
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
int getPid(int val);
int getValByPid(int pid)
{
    for (int i = 0; i < CHILD_COUNT; ++i)
    {
        if (getPid(i) == pid)
        {
            return i;
        }
    }
    return -1;
};
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
    printf("Signal sent from %d(value = %d) to %d(value = %d), signal: %s (at %lld)\n", fromPid, fromVal, toPid, toVal, sigStr, getTime());
}
void signalHandler(int sig, siginfo_t *siginfo, void *code)
{
    char sigStr[8];
    if (sig == SIGUSR1)
    {
        strcpy(sigStr, "SIGUSR1");
    }
    else if (sig == SIGUSR2)
    {
        strcpy(sigStr, "SIGUSR2");
    }
    printf("%d(%d) received %s from %d(%d) (at %lld)\n", getpid(), curNode->val, sigStr, siginfo->si_pid,
           getValByPid(siginfo->si_pid), getTime());

    if (sig == SIGUSR1)
    {
        usr1_count_rec++;
    }
    else if (sig == SIGUSR2)
    {
        usr2_count_rec++;
    }
    else if (sig == SIGTERM)
    {
        printf("[%lld]  %d(%d) got TERMINATED after sent: USR1: %d; USR2: %d;\n",
               getTime(),
               getpid(),
               curNode->val,
               usr1_count_snd,
               usr2_count_snd);
        exit(0);
    }
    if (curNode->val == 1 && usr1_count_rec == 101)
    {
        sendToAll(SIGTERM);
        while (wait(NULL) != -1)
            ;
        exit(0);
    }

    if (curNode->val == 1)
    {
        usr2_count_snd++;
        sendToAll(SIGUSR2);
    }
    else
    {
        for (int i = 0; i < CHILD_COUNT; ++i)
        {
            if (curNode->cn[i] != NULL)
            {

                int toVal = curNode->cn[i]->val;
                int toPid = getPid(toVal);
                printSignalSent(getpid(), curNode->val, toPid, SIGUSR1, toVal);
                usr2_count_snd++;
                if (kill(toPid, SIGUSR1))
                    perror("Can't kill");
            }
        }
    }
}

void sendToAll(int signal)
{
    __pid_t pgid = getpgid(getpid());
    printSignalSent(getpid(), curNode->val, (-1) * pgid, signal, -1);
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

int getPid(int val)
{
    key_t key = ftok("shmfile", 65);
    int shmid = shmget(key, 1024, 0666 | IPC_CREAT);
    int *str = (int *)shmat(shmid, (void *)0, 0);

    int ret = str[val];

    shmdt(str);
    return ret;
}

void createTree(Node *root)
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
        printf("Forked process %d(%d) from %d(%d)\n", getpid(), n->val, getppid(), curNode->val);
        curNode = n;
        savePid(curNode->val, getpid());
        establishSigHandler(curNode);

        return 0;
    default:
        return pid;
    }
}

int created[CHILD_COUNT] = {};

#pragma clang diagnostic push
#pragma ide diagnostic ignored "misc-no-recursion"
void createProcessTree(Node *root)
{
    created[root->val] = 1;
    curNode = root;
    for (int i = 0; i < CHILD_COUNT; ++i)
    {
        struct Node *nextNode = curNode->cn[i];
        if (nextNode != NULL && !created[nextNode->val])
        {
            char semName[255] = "/child_register_handler";
            size_t strlen1 = strlen(semName);
            semName[strlen1] = '0' + nextNode->val;
            semName[strlen1 + 1] = '\0';
            sem_t *childRegisterHandler = sem_open(semName, O_CREAT, 0777, 0);
            pid_t pid = forkProcess(nextNode);
            if (pid == 0)
            {
                // for child process
                createProcessTree(curNode);
                sem_post(childRegisterHandler);
                if (curNode->val == 1)
                {
                    sendToAll(SIGUSR2);
                }
                while (1)
                {
                    sleep(20000);
                }
                printf("Exiting %d\n", curNode->val);
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
#pragma clang diagnostic pop

int main(int argc, char *argv[])
{
    // strcpy(moduleName, argv[0]);
    Node *root = newNode(0, NULL, 0, 0);
    establishSigHandler(root);
    createTree(root);
    createProcessTree(root);
    while (wait(NULL) != -1)
        ;
    return 0;
}