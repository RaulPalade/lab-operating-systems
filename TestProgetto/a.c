#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <errno.h>
#include <time.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <sys/wait.h>

#define EXIT_ON_ERROR                                                                                           \
    if (errno) {                                                                                                \
        fprintf(stderr, "%d: pid %ld; errno: %d (%s)\n", __LINE__, (long)getpid(), errno, strerror(errno));     \
        raise(SIGINT);                                                                                          \
    }

void synchronize_resources(int);

int main() {
    key_t key;
    int semaphore;

    if ((key = ftok("./test.c", 'b')) < 0) {
        EXIT_ON_ERROR
    } else {
        printf("key=%d\n", key);
    }

    if ((semaphore = semget(key, 1, 0666)) < 0) {
        EXIT_ON_ERROR;
    } else {
        printf("semaphore=%d\n", semaphore);
    }

    synchronize_resources(semaphore);

    return 0;
}

void synchronize_resources(int semaphore) {
    struct sembuf sops;
    sops.sem_num = 0;
    sops.sem_op = 0;
    sops.sem_flg = 0;
    if (semop(semaphore, &sops, 1) < 0) {
        printf("Impossible to acquire semaphore\n");
        kill(getppid(), SIGUSR1);
        raise(SIGINT);
    }
}