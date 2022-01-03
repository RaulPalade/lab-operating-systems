#ifndef __UTIL_H_
#define __UTIL_H_
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
#include <sys/time.h>

#define SO_BLOCK_SIZE 2
#define SO_REGISTRY_SIZE 2
#define SENDER_TRANSACTION_REWARD -1

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

#define PROJ_ID_SHM_CONFIGURATION 'a'
#define PROJ_ID_SHM_LEDGER 'b'
#define PROJ_ID_SHM_NODE_LIST 'c'
#define PROJ_ID_SHM_USER_LIST 'd'
#define PROJ_ID_SHM_LAST_BLOCK_ID 'e'
#define PROJ_ID_SHM_LEDGER_SIZE 'f'

#define PROJ_ID_MSG_NODE_USER 'g'
#define PROJ_ID_MSG_USER_NODE 'h'

#define PROJ_ID_SEM_INIT 'i'
#define PROJ_ID_SEM_WRITERS 'l'

#define EXIT_ON_ERROR                                                                                           \
    if (errno) {                                                                                                \
        fprintf(stderr, "File %s %d: pid %ld; errno: %d (%s)\n", __FILE__, __LINE__, (long)getpid(), errno, strerror(errno));     \
        raise(SIGINT);                                                                                          \
    }

#define ARRAY_LEN(a) (sizeof(a) / sizeof((a)[0]))
typedef struct {
    int SO_USERS_NUM;
    int SO_NODES_NUM;
    int SO_REWARD;
    int SO_MIN_TRANS_GEN_NSEC;
    int SO_MAX_TRANS_GEN_NSEC;
    int SO_RETRY;
    int SO_TP_SIZE;
    int SO_MIN_TRANS_PROC_NSEC;
    int SO_MAX_TRANS_PROC_NSEC;
    int SO_BUDGET_INIT;
    int SO_SIM_SEC;
    int SO_FRIENDS_NUM;
    int SO_HOPS;
} configuration;

typedef struct {
    time_t timestamp;
    pid_t sender;
    pid_t receiver;
    int amount;
    int reward;
} transaction;

typedef struct {
    int id;
    transaction transactions[SO_BLOCK_SIZE];
} block;

typedef struct {
    block blocks[SO_REGISTRY_SIZE];
} ledger;

union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
    struct seminfo *__buf;
};

/* MESSAGE QUEUE STRUCTURES */
typedef struct {
    long mtype;
    transaction t;
} user_node_message;

typedef struct {
    long mtype;
    int args[2];
} node_master_message;

typedef struct {
    long mtype;
    int balance;
} user_master_message;

typedef struct {
    pid_t pid;
    int balance;
    int transactions_left;
} node_information;

typedef struct {
    pid_t pid;
    int balance;
} user_information;

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

/**
 * Function used by master process to release
 * the init semaphore to allow node and user
 * processes to begin execution
 */
void unlock_init_semaphore(int);

/**
 * Synchronyze resources before the simulations starts.
 * Node and user processes wait for 0 to begin the
 * execution after the master process ended resourse init
 */
void synchronize_resources(int);

int equal_transaction(transaction, transaction);

int array_contains(int [], int);

void print_configuration(configuration);

void print_transaction(transaction);

void print_block(block);

void print_all_transactions(transaction *);

void print_table_header();

void *new_shared_memory(char, int, size_t);

int new_message_queue(char);

int new_semaphore(char);

#endif