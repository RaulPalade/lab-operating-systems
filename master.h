#ifndef __MASTER_H_
#define __MASTER_H_
#include "util.h"

void print_configuration(configuration);

void print_all_transactions(transaction *);

void print_block(block *);

void print_ledger(ledger *);

void init_ledger();

void execute_node();

void execute_user();

void print_live_ledger_info();

void print_final_report();

void handler(int);

int add_to_block(block *, transaction);

int remove_from_block(block *, transaction);

int add_to_ledger(block block);

int remove_from_ledger(block block);

#endif