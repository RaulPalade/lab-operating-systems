#ifndef __MASTER_H_
#define __MASTER_H_
#include "util.h"

void print_configuration(configuration);

void print_transaction(transaction *);

void print_all_transactions(transaction *);

void print_block(block *);

void print_ledger(ledger *);

void init_ledger();

void execute_node();

void execute_user();

void print_live_ledger_info(ledger *);

void print_final_report();

void handler(int);

char * get_status(transaction);

void print_table_header();

int add_to_block(block *, transaction *transaction);

int add_to_ledger(block block);

#endif