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

void read_configuration(configuration *);

void cleanIPC();

void handler(int);

#endif