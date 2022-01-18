#include <sys/sem.h>
#include "../include/semlib.h"

int set_semaphore_val(int id_semaphore, int sem_num, int sem_val) {
    return semctl(id_semaphore, sem_num, SETVAL, sem_val);
}

int lock(int semaphore) {
    struct sembuf sops;
    sops.sem_num = 0;
    sops.sem_op = -1;
    sops.sem_flg = 0;
    return semop(semaphore, &sops, 1);
}

int unlock(int semaphore) {
    struct sembuf sops;
    sops.sem_num = 0;
    sops.sem_op = 1;
    sops.sem_flg = 0;
    return semop(semaphore, &sops, 1);
}

int unlock_init_semaphore(int semaphore) {
    struct sembuf sops;
    sops.sem_num = 0;
    sops.sem_op = -1;
    sops.sem_flg = 0;
    return semop(semaphore, &sops, 1);
}

int wait_for_master(int semaphore) {
    struct sembuf sops;
    sops.sem_num = 0;
    sops.sem_op = 0;
    sops.sem_flg = 0;
    return semop(semaphore, &sops, 1);
}