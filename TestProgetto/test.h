#ifndef __TEST_H_
#define __TEST_H_
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

#define SO_BLOCK_SIZE 5
#define SO_REGISTRY_SIZE 100
#define SO_TP_SIZE 10
#define SENDER_TRANSACTION_REWARD -1

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

#define EXIT_ON_ERROR                                                                                           \
    if (errno) {                                                                                                \
        fprintf(stderr, "%d: pid %ld; errno: %d (%s)\n", __LINE__, (long)getpid(), errno, strerror(errno));     \
        raise(SIGINT);                                                                                          \
    }

enum transaction_status {UNKNOWN, PROCESSING, COMPLETED, ABORTED};

enum {SECS_TO_SLEEP = 1, NSEC_TO_SLEEP = 500000000};

typedef struct {
    int SO_USERS_NUM;
    int SO_NODES_NUM;
    int SO_REWARD;
    int SO_MIN_TRANS_GEN_NSEC;
    int SO_MAX_TRANS_GEN_NSEC;
    int SO_RETRY;
    int SO_MIN_TRANS_PROC_NSEC;
    int SO_MAX_TRANS_PROC_NSEC;
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
    enum transaction_status status;
} transaction;

typedef struct {
    int id;
    transaction transactions[SO_BLOCK_SIZE];
} block;

typedef struct {
    block blocks[SO_REGISTRY_SIZE];
} ledger;

typedef struct {
    transaction transactions[SO_TP_SIZE];
} transaction_pool;

typedef struct {
    int last_block_id;
    int balance;
} user_balance;

union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
    struct seminfo *__buf;
};

/* UTIL FUNCTIONS */
void read_configuration(configuration *); /* MASTER */

char * get_status(transaction); /* NODE-USER */
/* END UTIL FUNCTIONS */


/* MASTER FUNCTIONS */
void execute_node();

void execute_user();

int handler(int);
/* END MASTER FUNCTIONS */


/* NODE FUNCTIONS */
int add_to_transaction_pool(transaction);

int remove_from_transaction_pool(transaction);

int add_to_block(block *, transaction);

int remove_from_block(block *, transaction);

int execute_transaction(transaction *);

int ledger_has_transaction(ledger *, transaction);

int add_to_ledger(ledger *, block block);

int remove_from_ledger(ledger *, block block);

block new_block();

transaction new_reward_transaction(pid_t, int);

transaction get_random_transaction();
/* END NODE FUNCTIONS */


/* USER FUNCTIONS */
transaction new_transaction(pid_t, int, int);

int calculate_balance();

pid_t get_random_user();

pid_t get_random_node();
/* END USER FUNCTIONS */


/* PRINT FUNCTIONS */
void print_configuration(configuration);

void print_transaction(transaction);

void print_block(block *);

void print_ledger(ledger *);

void print_all_transactions(transaction *);

void print_transaction_pool();

void print_table_header();

void print_live_ledger_info(ledger *);

void print_final_report();
/* END PRINT FUNCTIONS */

/* SEMAPHORE FUNCTIONS */
void synchronize_resources(int);

void acquire_resource(int, int);

void release_resource(int, int);

void lock(int);

void unlock(int);

void unblock(int);
/* END SEMAPHORE FUNCTIONS */
#endif