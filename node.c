#include "util.h"
#include "node.h"

ledger (*master_ledger);
transaction_pool pool;

static int ledger_size = 0;
static int transaction_pool_size = 0;
static int balance = 100;
static int block_id = 0;
static int next_block_to_check = 0;

/**
 * NODE PROCESS
 * 1) Receive transaction from User                     
 * 2) Add transaction received to transaction pool      
 * 3) Add transaction to block                          
 * 4) Add reward transaction to block                   
 * 5) Execute transaction                               
 * 6) Remove transactions from transaction pool         
 */

int main() {
    return 0;
}

int add_to_transaction_pool(transaction t) {
    int added = 0;
    if (transaction_pool_size < SO_TP_SIZE - 1) {
        pool.transactions[transaction_pool_size] = t;
        transaction_pool_size++;
        added = 1;
    } else {
        printf(ANSI_COLOR_RED "Transaction pool size exceeded\n" ANSI_COLOR_RESET);
    }

    return added;
}

int remove_from_transaction_pool(transaction t) {
    int removed = 0;
    int i;
    int position;
    for (i = 0; i < transaction_pool_size && !removed; i++) {
        if (pool.transactions[i].timestamp == t.timestamp && pool.transactions[i].sender == t.sender
            && pool.transactions[i].receiver == t.receiver) {
            for (position = i; position < transaction_pool_size; position++) {
                pool.transactions[position] = pool.transactions[position + 1];
            }
            transaction_pool_size--;
            removed = 1;
        }
    }

    return removed;
}

int execute_transaction(transaction *t) {
    int executed = 0;
    struct timespec interval;
    interval.tv_sec = 1;
    interval.tv_nsec = 0;
    printf("Executing transaction...\n");
    nanosleep(&interval, NULL);
    (*t).status = COMPLETED;

    return executed;
}

int ledger_has_transaction(ledger *ledger, transaction t) {
    int found = 0;
    int i;
    int j;

    for (i = 0; i < ledger_size && !found; i++) {
        for (j = 0; j < SO_BLOCK_SIZE; j++) {
            if ((*ledger).blocks[i].transactions[j].timestamp == t.timestamp &&
                (*ledger).blocks[i].transactions[j].sender == t.sender) {
                found = 1;
            }
        }
        next_block_to_check++;
    }

    return found;
}

int add_to_ledger(ledger *ledger, block block) {
    int added = 0;
    if (ledger_size < SO_REGISTRY_SIZE) {
        (*ledger).blocks[ledger_size] = block;
        ledger_size++;
        added = 1;
    } else {
        printf(ANSI_COLOR_RED "Ledger size exceeded\n" ANSI_COLOR_RESET);
    }

    return added;
}

block new_block(transaction transactions[]) {
    int i;
    block block;
    block.id = block_id;
    for (i = 0; i < SO_BLOCK_SIZE; i++) {
        block.transactions[i] = transactions[i];
    }
    block_id++;

    return block;
}

transaction new_reward_transaction(pid_t receiver, int amount) {
    transaction transaction;
    transaction.timestamp = time(NULL);
    transaction.sender = SENDER_TRANSACTION_REWARD;
    transaction.receiver = 999999;
    transaction.amount = amount;
    transaction.reward = 0;

    return transaction;
}

transaction *extract_transaction_block_from_pool() {
    int i;
    int confirmed = 0;
    int lower = 0;
    int upper = transaction_pool_size;
    int random = 0;
    transaction *transactions = malloc(SO_BLOCK_SIZE * sizeof(transaction));
    int numbers[SO_BLOCK_SIZE];

    srand(time(NULL));

    while (confirmed < SO_BLOCK_SIZE) {
        random = (rand() % (upper - lower)) + lower;
        if (!array_contains(numbers, random)) {
            numbers[confirmed] = random;
            transactions[confirmed] = pool.transactions[random];
            confirmed++;
        }
    }

    return transactions;
}

void print_transaction_pool() {
    int i;
    print_table_header();
    for (i = 0; i < transaction_pool_size; i++) {
        print_transaction(pool.transactions[i]);
    }
}

void reset_transaction_pool() {
    memset(&pool, 0, sizeof(pool));
    transaction_pool_size = 0;
}

void reset_ledger(ledger *ledger) {
    memset(master_ledger->blocks, 0, sizeof(master_ledger->blocks));
    ledger_size = 0;
    next_block_to_check = 0;
    block_id = 0;
}