#include "util.h"

#define SENDER_TRANSACTION_REWARD -1

typedef struct {
    transaction transactions[SO_TP_SIZE];
} transaction_pool;

int add_to_transaction_pool(transaction);

int remove_from_transaction_pool(transaction);

int execute_transaction(transaction *);

int ledger_has_transaction(ledger *, transaction);

int add_to_ledger(ledger *, block block);

block new_block();

transaction new_reward_transaction(int);

transaction *extract_transactions_block_from_pool();

void print_transaction_pool();

void reset_transaction_pool();

void reset_ledger(ledger *);

void handler(int);