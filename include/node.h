#include "util.h"
#include "semlib.h"

#define SENDER_TRANSACTION_REWARD -1

typedef struct {
    transaction *transactions;
} transaction_pool;

int add_to_transaction_pool(transaction);

int remove_from_transaction_pool(transaction);

transaction *extract_transactions_block_from_pool();

block new_block(transaction []);

transaction new_reward_transaction(unsigned long);

int add_to_ledger(ledger *, block block);

int ledger_has_transaction(ledger *, transaction);

void print_transaction_pool();

void update_info();

void clean_transaction_pool();

void handler(int);