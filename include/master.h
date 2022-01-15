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
} user_data;

int calculate_user_balance(pid_t user);

int calculate_node_balance(pid_t node);

void print_ledger();

void print_live_info();

void print_final_report();

void read_configuration(configuration *);

void cleanIPC();

void handler(int);

void add_max(user_data *, pid_t, int);

void add_min(user_data *, pid_t, int);

void init_array(user_data *, int);

void print_user_data(user_data *);

int compare(const void *, const void *);

#endif