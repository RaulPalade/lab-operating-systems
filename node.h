#include "util.h"

enum {SECS_TO_SLEEP = 3, NSEC_TO_SLEEP = 0};

typedef struct {
    transaction transactions[SO_TP_SIZE];
} transaction_pool;

void print_transaction_pool();

int add_transaction_to_pool(transaction);

int remove_from_transaction_pool(transaction);

void execute_transaction(transaction *);

void check_transaction_in_ledger(transaction);