#include "util.h"
#include "node.h"

transaction_pool pool;
static int transaction_pool_size = 0;
struct timespec remaining;
struct timespec request = {SECS_TO_SLEEP, NSEC_TO_SLEEP};
ledger master_ledger;
static int ledger_size = 0;
static int block_size = 0;
static int block_id = 1;



/**
 * HOW IT WORKS?
 * 1) Receive transaction from User                     => MESSAGE QUEUE
 * 2) Add transaction received to transaction pool      => DONE
 * 3) Add transaction to block                          => DONE
 * 4) Add reward transaction to block                   => DONE
 * 5) Execute transaction                               => SIMULATING
 * 6) Remove transactions from transaction pool         => DONE
 */


int main() {
    int result = test_lifecycle();

    if (result) {
        printf("CORRECT\n");
    } else {
        printf("WRONG\n");
    }

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
        if (pool.transactions[i].timestamp == t.timestamp && pool.transactions[i].sender == t.sender) {
            for (position = i; position < transaction_pool_size; position++) {
                pool.transactions[position] = pool.transactions[position + 1];
            }
            transaction_pool_size--;
            removed = 1;
        }
    }

    return removed;
}

int add_to_block(block *block, transaction t) {
    int added = 0;
    if (block_size < SO_BLOCK_SIZE) {
        (*block).transactions[block_size] = t;
        block_size++;
        added = 1;
    } else {
        printf(ANSI_COLOR_RED "Block size exceeded\n" ANSI_COLOR_RESET);
    }

    printf("Transactions in the block: %d\n", block_size);
    printf("Transactions available for block with id %d:  %d\n", (*block).id, SO_REGISTRY_SIZE - block_size);

    return added;
}

int remove_from_block(block *block, transaction t) {
    int removed = 0;
    int i;
    int position;
    for (i = 0; i < block_size && !removed; i++) {
        if ((*block).transactions[i].timestamp == t.timestamp && (*block).transactions[i].sender == t.sender) {
            for (position = i; position < block_size; position++) {
                (*block).transactions[position] = (*block).transactions[position + 1];
            }
            block_size--;
            removed = 1;
        }
    }

    return removed;
}

// USEFUL
int execute_transaction(transaction *t) {
    int executed = 0;
    printf("Executing transaction...\n");
    nanosleep(&request, &remaining);
    (*t).status = COMPLETED;

    return executed;
}

int ledger_has_transaction(transaction t) {
    int found = 0;
    int i;
    int j;

    for (i = 0; i < ledger_size && !found; i++) {
        for (j = 0; j < block_size; j++) {
            if (master_ledger.blocks[i].transactions[j].timestamp == t.timestamp &&
                master_ledger.blocks[i].transactions[j].sender == t.sender) {
                found = 1;
            }
        }
    }

    return found;
}

int add_to_ledger(block block) {
    int added = 0;
    if (ledger_size < SO_REGISTRY_SIZE) {
        master_ledger.blocks[ledger_size] = block;
        ledger_size++;
        added = 1;
    } else {
        printf(ANSI_COLOR_RED "Ledger size exceeded\n" ANSI_COLOR_RESET);
    }

    printf("Blocks in the ledger: %d\n", ledger_size);
    printf("Blocks available: %d\n", SO_REGISTRY_SIZE - ledger_size);

    return added;
}

int remove_from_ledger(block block) {
    int removed = 0;
    int i;
    int position;
    for (i = 0; i < block_size && !removed; i++) {
        if (master_ledger.blocks[i].id == block.id) {
            for (position = i; position < block_size; position++) {
                master_ledger.blocks[position] = master_ledger.blocks[position + 1];
            }
            ledger_size--;
            removed = 1;
        }
    }

    return removed;
}

block new_block() {
    block block;
    block.id = block_id;
    block_id++;
}

// NEED TO BE MODIFIED (MOVE TO USER PROCESS)
transaction new_transaction(pid_t receiver, int amount, int reward) {
    transaction transaction;
    transaction.timestamp = time(NULL);
    transaction.sender = getpid();
    transaction.receiver = receiver;
    transaction.amount = amount;
    transaction.reward = reward;
    transaction.status = PROCESSING;
    nanosleep(&request, &remaining);

    add_to_transaction_pool(transaction);
    
    return transaction;
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

// NEED TO CORRECT IF CHECK
transaction get_random_transaction() {
    transaction transaction;
    int lower = 0;
    int upper = transaction_pool_size;
    int random = 0;
    srand(time(0));
    random = (rand() % (upper - lower + 1)) + lower;

    if (ledger_has_transaction(pool.transactions[random])) {
        get_random_transaction();
    }
    
    transaction = pool.transactions[random];
    return transaction;
}

// TO REMOVE
int test_lifecycle() {
    int correct = 0;
    int added_to_block = 0;
    int added_to_ledger = 0;
    
    printf("Creating new transactions...\n");
    transaction t1 = new_transaction(222222, 20, 5);
    transaction t2 = new_transaction(444444, 60, 15);
    transaction t3 = new_transaction(666666, 30, 10);

    transaction random_transaction = get_random_transaction();
    transaction reward_transaction = new_reward_transaction(777777, 20);
    
    printf("\n");
    printf("Adding transaction to block...\n");
    nanosleep(&request, &remaining);
    block block = new_block();
    added_to_block = add_to_block(&block, random_transaction);
    added_to_block = add_to_block(&block, reward_transaction);

    printf("\n");
    printf("Adding block to ledger...\n");
    nanosleep(&request, &remaining);
    added_to_ledger = add_to_ledger(block);

    printf("\n");
    printf("Removing transaction completed from pool...\n");
    nanosleep(&request, &remaining);

    print_transaction_pool();
    if (added_to_block && added_to_ledger) {
        correct = remove_from_transaction_pool(random_transaction);
    }
    print_transaction_pool();

    return correct;
}

void print_transaction_pool() {
    int i;
    print_table_header();
    for (i = 0; i < transaction_pool_size; i++) {
        print_transaction(pool.transactions[i]);
    }
}