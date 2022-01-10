#include <sys/sem.h>
#include "../include/semlib.h"

int set_semaphore_val(int id_semaphore, int sem_num, int sem_val) {
    return semctl(id_semaphore, sem_num, SETVAL, sem_val);
}

/* For writers and also for readers when readers == 1 */
int acquire_resource(int semaphore, int id_block) {
    struct sembuf sops;
    sops.sem_num = id_block;
    sops.sem_op = -1;
    sops.sem_flg = SEM_UNDO;
    return semop(semaphore, &sops, 1);
}

/* For writers and also for readers when readers == 0 */
int release_resource(int semaphore, int id_block) {
    struct sembuf sops;
    sops.sem_num = id_block;
    sops.sem_op = 1;
    sops.sem_flg = SEM_UNDO;
    return semop(semaphore, &sops, 1);
}

/* Mutex for readers
 * lock(mutex) mutex protect numLettori
 * numLettori++;
 * if(numLettori == 1)
 *   acquire_resource(semaphore, id_block)
 * unlock(mutex)
 *
 * P(mutex)
 * numLettori--;
 * if(numLettori == 0)
 *   release_resource(semaphore, id_block)
 * unlock(mutex)
 * */
int lock(int semaphore) {
    struct sembuf sops;
    sops.sem_num = 0;
    sops.sem_op = -1;
    sops.sem_flg = 0;
    return semop(semaphore, &sops, 1);
}

/* Mutex for readers */
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

/* Initial semaphore used to init all resources by the master process */
int wait_for_master(int semaphore) {
    struct sembuf sops;
    sops.sem_num = 0;
    sops.sem_op = 0;
    sops.sem_flg = 0;
    return semop(semaphore, &sops, 1);
}