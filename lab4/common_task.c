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

typedef struct Node {
	int val;
	struct Node *cn[CHILD_COUNT];
	int signal; // -1 - no signal
	int signal2; // -1 - no signal
	sigset_t *sigset;
} Node;

Node *curNode = NULL;
int usr1_count_rec = 0;
int usr2_count_rec = 0;
int usr1_count_snd = 0;
int usr2_count_snd = 0;

long long getTime() {
	struct timeval tv;

	if (gettimeofday(&tv, NULL)==-1) {
		perror("Can not get current time");
		return -1;
	}

	return tv.tv_sec*1000000 + tv.tv_usec;
}

void sendToAll(int signal);
int getPid(int val);
int getValByPid(int pid) {
	for (int i = 0; i < CHILD_COUNT; ++i) {
		if (getPid(i)==pid) {
			return i;
		}
	}
	return -1;
};

void printSignalSent(int fromPid, int fromVal, int toPid, int signal, int toVal) {
	printf("[%lld]  Signal sent from %d(%d) to %d(%d), signal: %d\n", getTime(), fromPid, fromVal, toPid, toVal, signal);
}

void signalHandler(int sig, siginfo_t *siginfo, void *code) {
	printf("[%lld]  %d(%d) received signal %d from %d(%d)\n", getTime(), getpid(), curNode->val, sig, siginfo->si_pid,
	       getValByPid(siginfo->si_pid));

	if (sig==SIGUSR1) {
		usr1_count_rec++;
	} else if (sig==SIGUSR2) {
		usr2_count_rec++;
	} else if (sig==SIGTERM) {
		printf("[%lld]  %d(%d) received SIGTERM after sent: USR1: %d; USR2: %d;\n",
		       getTime(),
		       getpid(),
		       curNode->val,
		       usr1_count_snd,
		       usr2_count_snd);
		exit(0);
	}
//	printf("USR1 count: %d\n", usr1_count_rec);
//	printf("USR2 count: %d\n", usr2_count_rec);
	if (curNode->val==1) {
		usr2_count_snd++;
		sleep(1);
		sendToAll(SIGUSR1);
	} else {
		for (int i = 0; i < CHILD_COUNT; ++i) {
			if (curNode->cn[i]!=NULL) {

				int toVal = curNode->cn[i]->val;
				int toPid = getPid(toVal);
				printSignalSent(getpid(), curNode->val, toPid, SIGUSR2, toVal);
				usr2_count_snd++;
				if(kill(toPid, SIGUSR2))
					perror("Can't kill");
			}
		}
	}
}

void sendToAll(int signal) {
	__pid_t pgid = getpgid(getpid());
	printSignalSent(getpid(), curNode->val, (-1)*pgid, signal, -1);
	if (kill((-1)*pgid, signal))
		perror("Can't send signal");
}

Node *createNode(int val, Node *parent, int signal, int signal2) {
	Node *n = (Node *)malloc(sizeof(Node));
	n->signal = signal;
	n->signal2 = signal2;
	n->val = val;
	n->sigset = (sigset_t *)malloc(sizeof(sigset_t));
	for (int i = 0; i < CHILD_COUNT; ++i) {
		n->cn[i] = NULL;
	}
	if (parent!=NULL) {
		char nodeAdded = 0;
		for (int i = 0; i < CHILD_COUNT; ++i) {
			if (parent->cn[i]==NULL) {
				parent->cn[i] = n;
				nodeAdded = 1;
				break;
			}
		}
		if (!nodeAdded) {
			perror("Error: node not added\n");
		}
	}
	return n;
}

void savePid(int val, int pid) {
	key_t key = ftok("shmfile", 65);
	int shmid = shmget(key, 1024, 0666 | IPC_CREAT);
	int *str = (int *)shmat(shmid, (void *)0, 0);

	str[val] = pid;

	shmdt(str);
}

int getPid(int val) {
	key_t key = ftok("shmfile", 65);
	int shmid = shmget(key, 1024, 0666 | IPC_CREAT);
	int *str = (int *)shmat(shmid, (void *)0, 0);

	int ret = str[val];

	shmdt(str);
	return ret;
}

void createTree(Node *root) {
	Node *n1 = createNode(1, root, SIGUSR2, 0);
	Node *n2 = createNode(2, n1, SIGUSR1, 0);
	Node *n3 = createNode(3, n1, SIGUSR1, 0);
	n2->cn[0] = n1;
	n3->cn[0] = n1;
}

void registerSignalHandler(Node *n) {
	if (n->signal==-1) return;

	if (sigfillset(n->sigset)) {
		perror("Error: filling set");
	}
	if (n->signal!=0) {
		if (sigdelset(n->sigset, n->signal)) {
			perror("Error: deleting set");
		}
	}
	if (n->signal2!=0) {
		if (sigdelset(n->sigset, n->signal2)) {
			perror("Error: deleting set");
		}
	}
	if (sigprocmask(SIG_SETMASK, n->sigset, NULL) < 0) {
		perror("Error: setting signal mask");
	}

	struct sigaction act;
	memset(&act, 0, sizeof(act));
	act.sa_sigaction = signalHandler;
	act.sa_mask = *(n->sigset);
	act.sa_flags = SA_SIGINFO;
	if (n->signal && sigaction(n->signal, &act, 0)) {
		perror("Error: registering signal handler");
	}

	if (n->signal2 && sigaction(n->signal2, &act, 0)) {
		perror("Error: registering signal handler");
	}
}

pid_t forkProcess(Node *n) {
	pid_t pid = fork();
	switch (pid) {
		case -1: perror("Error: fork failed\n");
			break;
		case 0:
			// for child process
			printf("Forked process %d(%d) from %d(%d)\n", getpid(), n->val, getppid(), curNode->val);
			curNode = n;
			savePid(curNode->val, getpid());
			registerSignalHandler(curNode);

			return 0;
		default:
			// for parent process
			return pid;
	}
}

int created[CHILD_COUNT] = {};

#pragma clang diagnostic push
#pragma ide diagnostic ignored "misc-no-recursion"
void createProcessTree(Node *root) {
	created[root->val] = 1;
	curNode = root;
	for (int i = 0; i < CHILD_COUNT; ++i) {
		struct Node *nextNode = curNode->cn[i];
//		printf("In cycle %d\n", nextNode->val);
		if (nextNode!=NULL && !created[nextNode->val]) {
			char semName[255] = "/child_register_handler";
			size_t strlen1 = strlen(semName);
			semName[strlen1] = '0' + nextNode->val;
			semName[strlen1 + 1] = '\0';
//			printf("semName: %s\n", semName);
			sem_t *childRegisterHandler = sem_open(semName, O_CREAT, 0777, 0);

//			sem_init(childRegisterHandler, 1, 0);
			pid_t pid = forkProcess(nextNode);
//			printf("Creating process tree for %d\n", curNode->val);
			if (pid==0) {
				// for child process //
//				printf("Creating process tree for %d\n", curNode->val);
				createProcessTree(curNode);
				sem_post(childRegisterHandler);
				printf("Curnode.val: %d\n", curNode->val);
				if (curNode->val==1) {
					sendToAll(SIGUSR1);
				}
				while (1) { sleep(20000); }
//				while (1){};
//				sleep(20000);
				printf("Exiting %d\n", curNode->val);
				exit(0);
			} else {
				// for parent process //
				sem_wait(childRegisterHandler);
				printf("Process tree for %d created\n", nextNode->val);
//				sem_destroy(childRegisterHandler);
//				if (nextNode->signal!=-1 && curNode->val==1) {
//					printf("NexNode %d\n", nextNode->val);
//					printf("CurNode %d\n", curNode->val);
//					printf("Killing %d\n", nextNode->val);
//					kill(pid, SIGUSR2);
//					printf("Killed %d\n", nextNode->val);
//				}
//				printf("Before continue\n");
				continue;
			}
		}
	}

	// TODO: wait for all child processes check
//	while (1){};
//	while (wait(NULL)!=-1);
//	printf("Finished %d\n", curNode->val);
}
#pragma clang diagnostic pop

int main() {
	Node *root = createNode(0, NULL, 0, 0);
	registerSignalHandler(root);
	createTree(root);
	createProcessTree(root);

//	kill(-getpid(), SIGUSR2);
	while (wait(NULL)!=-1);
	return 0;
}