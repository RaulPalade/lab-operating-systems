#ifndef __MASTER_H_
#define __MASTER_H_

#include "util.h"
#include "semlib.h"

#define NODE "node"
#define USER "user"

int calculate_user_balance(pid_t user);

int calculate_node_balance(pid_t node);

void print_ledger();

void print_live_info();

void print_final_report();

void read_configuration(configuration *);

void cleanIPC();

void handler(int);

#endif