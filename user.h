#include "util.h"

transaction new_transaction(pid_t, int, int);

int calculate_balance();

pid_t get_random_user();

pid_t get_random_node();

int remove_from_processing_list(int);

int add_to_processing_list(transaction t);

int add_to_completed_list(transaction t);

void print_processing_list();

void print_completed_list();