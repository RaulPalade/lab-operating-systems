#include "util.h"

typedef struct {
    transaction transactions[10];
} transaction_pool;

void add_transaction_to_pool(transaction *);

void execute_transaction(transaction *);

void check_transaction_in_ledger(transaction *);

void remove_from_transaction_pool(transaction *);