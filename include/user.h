#include "util.h"

int calculate_balance();

transaction new_transaction();

void add_to_processing_list(transaction t);

void remove_from_processing_list(int);

void add_to_completed_list(transaction t);

pid_t get_random_user();

pid_t get_random_node();

void print_processing_list();

void print_completed_list();

void update_info();

void handler(int);