#include "util.h"

transaction new_transaction();

int calculate_balance();

pid_t get_random_user();

pid_t get_random_node();

void remove_from_processing_list(int);

void add_to_processing_list(transaction t);

void add_to_completed_list(transaction t);

void print_processing_list();

void print_completed_list();

void handler(int);

void update_info(int);