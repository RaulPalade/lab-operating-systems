#ifndef __MASTER_H_
#define __MASTER_H_

#include "util.h"
#include "semlib.h"

#define NODE "node"
#define USER "user"

void print_ledger(ledger *);

void print_live_info();

void print_node_info();

void print_user_info();

void print_final_report();

configuration read_configuration();

void cleanIPC();

void handler(int);

#endif