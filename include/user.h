#include "util.h"
#include "semlib.h"

int calculate_balance();

transaction new_transaction();

void execute_transaction();

void add_to_processing_list(transaction t);

void remove_from_processing_list(int);

pid_t get_random_user();

void print_processing_list();

void update_info();

void die();

void handler(int);