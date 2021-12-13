#include "util.h"

transaction new_transaction(pid_t, int, int);

static int calculate_balance();

pid_t get_random_user();

pid_t get_random_node();

int handler(int);

int add_to_block(block *, transaction);

int add_to_ledger(block block);