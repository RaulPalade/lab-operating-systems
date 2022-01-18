#ifndef __MASTER_H_
#define __MASTER_H_

#include <limits.h>
#include "util.h"
#include "semlib.h"

#define NODE "node"
#define USER "user"

#define N_USER_TO_DISPLAY 10

typedef struct {
    pid_t pid;
    int balance;
} child_data;

int calculate_user_balance(pid_t user, int);

int calculate_node_balance(pid_t node, int);

void print_ledger();

void print_final_report();

void read_configuration(configuration *);

void cleanIPC();

void handler(int);

void add_max(child_data *, pid_t, int);

void add_min(child_data *, pid_t, int);

void init_array(child_data *, int);

void shuffle(int *);

pid_t *get_random_friends(pid_t node);

void print_live_info(child_data *, child_data *, child_data *);

int compare(const void *, const void *);

void check_semaphore();

#endif