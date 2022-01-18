#ifndef __SEMLIB_H
#define __SEMLIB_H

union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
    struct seminfo *__buf;
};

int set_semaphore_val(int, int, int);

int lock(int);

int unlock(int);

int unlock_init_semaphore(int);

int wait_for_master(int);

#endif