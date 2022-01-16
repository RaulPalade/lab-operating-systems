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
#include <sys/time.h>
#include <assert.h>

/*#define SO_BLOCK_SIZE 100
#define SO_REGISTRY_SIZE 1000*/

#define SO_BLOCK_SIZE 10
#define SO_REGISTRY_SIZE 10000

/*#define SO_BLOCK_SIZE 10
#define SO_REGISTRY_SIZE 1000*/

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
#define PROJ_ID_SHM_BLOCK_ID 'e'
#define PROJ_ID_SHM_READERS_BLOCK_ID 'f'

#define PROJ_ID_MSG_NODE_USER 'g'
#define PROJ_ID_MSG_USER_NODE 'h'

#define PROJ_ID_SEM_INIT 'i'
#define PROJ_ID_SEM_WRITERS 'l'
#define PROJ_ID_SEM_READERS_BLOCK_ID 'm'

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

/* MESSAGE QUEUE STRUCTURES */
typedef struct {
    long mtype;
    transaction t;
} user_node_message;

int equal_transaction(transaction, transaction);

int array_contains(transaction *, transaction);

void print_configuration(configuration);

void print_table_header();

void print_transaction(transaction);

void print_block(block);

void print_all_transactions(transaction *);

long get_timestamp_millis();

#endif