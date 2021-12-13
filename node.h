#include "util.h"

#define SENDER_TRANSACTION_REWARD -1

typedef struct {
    transaction transactions[SO_TP_SIZE];
} transaction_pool;

int add_to_transaction_pool(transaction);

int remove_from_transaction_pool(transaction);

int add_to_block(block *, transaction);

int remove_from_block(block *, transaction);

int execute_transaction(transaction *);

int ledger_has_transaction(transaction);

int add_to_ledger(block block);

int remove_from_ledger(block block);

block new_block();

transaction new_reward_transaction(pid_t, int);

transaction get_random_transaction();

//int test_lifecycle();

void print_transaction_pool();