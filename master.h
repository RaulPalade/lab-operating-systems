#ifndef __MASTER_H_
#define __MASTER_H_

#include "util.h"

void execute_node(int);

void execute_user(int);

void handler(int);

void print_ledger(ledger *);

void print_live_info();

void print_node_info();

void print_user_info();

void print_final_report();

/**
 * Read the initial configuration from file
 */
configuration read_configuration();

#endif