#ifndef __SEMLIB_H
#define __SEMLIB_H

union semun {
    int val;    /* Value for SETVAL */
    struct semid_ds *buf;    /* Buffer for IPC_STAT, IPC_SET */
    unsigned short *array;  /* Array for GETALL, SETALL */
    struct seminfo *__buf;  /* Buffer for IPC_INFO
                                           (Linux-specific) */
};

int set_semaphore_val(int, int, int);

/**
 * Function used by processes to access a
 * protected resource in order to
 * avoid data inconsistency.
 *
 * If a resource wants to write on a
 * shared memory segment, it must acquire
 * the resource in order to stop other processes
 * accessing it during writing operations.
 *
 * If a resource wants to read on a
 * shared memory segment, it must acquire the
 * resource in order to stop other writer
 * processes to access it during reading operations.
 *
 * If there are many processes that wants to read
 * in the segment, only the first process will block the resource.
 * Condition readers == 1
 *
 * @param semaphore semaphore used to protect the resource
 * @param id_block  id of the block to access
 */
int acquire_resource(int, int);

/**
 * Function used by processes to release a
 * protected resource in order to
 * leave it available to other processes.
 *
 * If the process was acquired for read operations,
 * the last process must realease the resource
 * Condition readers == 0
 *
 * @param semaphore semaphore used to protect the resource
 * @param id_block  id of the block to release
 */
int release_resource(int, int);

/**
 * Function used by reader processes to
 * increment the number of processes that
 * are currently reading the proteted resource
 * No other process can overwrite the variable
 * if one is doing it.
 */
int lock(int);

/**
 * Function used by reader processes to
 * decrement the number of processes that
 * are currently reading the proteted resource
 * No other process can overwrite the variable
 * if one is doing it.
 */
int unlock(int);

/**
 * Function used by master process to release
 * the init semaphore to allow node and user
 * processes to begin execution
 */
int unlock_init_semaphore(int);

/**
 * Synchronyze resources before the simulations starts.
 * Node and user processes wait for 0 to begin the
 * execution after the master process ended resourse init
 */
int wait_for_master(int);

#endif