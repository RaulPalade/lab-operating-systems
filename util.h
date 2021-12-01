#ifndef __UTIL_H_
#define __UTIL_H_
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

#define SO_BLOCK_SIZE 5
#define SO_REGISTRY_SIZE 100

#define EXIT_ON_ERROR                                                                                           \
    if (errno) {                                                                                                \
        fprintf(stderr, "%d: pid %ld; errno: %d (%s)\n", __LINE__, (long)getpid(), errno, strerror(errno));     \
        raise(SIGINT);                                                                                          \
    }

typedef struct {
    int SO_USERS_NUM;
    int SO_NODES_NUM;
    int SO_REWARD;
    int SO_MIN_TRANS_GEN_NSEC;
    int SO_MAX_TRANS_GEN_NSEC;
    int SO_RETRY;
    int SO_TP_SIZE;
    /* int SO_BLOCK_SIZE; */
    int SO_MIN_TRANS_PROC_NSEC;
    int SO_MAX_TRANS_PROC_NSEC;
    /* int SO_REGISTRY_SIZE; */
    int SO_BUDGET_INIT;
    int SO_SIM_SEC;
    int SO_FRIENDS_NUM;
} configuration;

typedef struct {
    time_t timestamp;
    pid_t sender;
    pid_t receiver;
    int amount;
    int reward;
    int completed;
} transaction;

typedef struct {
    int id;
    transaction transactions[SO_BLOCK_SIZE];
} block;

typedef struct {
    block blocks[SO_REGISTRY_SIZE];
} ledger;

/**
 * Read the initial configuration from file
 */
void read_configuration(configuration *);

/**
 * Synchronyze resources before the simulations starts.
 * Used by the master process in order to:
 * - Create Node processes
 * - Create User processes
 * - Initialize the ledger as a shared memory
 * - Assign a default budget for each user
 * - Execute the first transaction for each user
 */ 
void synchronize_resources(int);

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
void acquire_resource(int, int);

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
void release_resource(int, int);

/**
 * Function used by reader processes to 
 * increment the number of processes that 
 * are currently reading the proteted resource
 * No other process can overwrite the variable
 * if one is doing it.
 */ 
void lock(int);

/**
 * Function used by reader processes to 
 * decrement the number of processes that 
 * are currently reading the proteted resource
 * No other process can overwrite the variable
 * if one is doing it.
 */ 
void unlock(int);

#endif