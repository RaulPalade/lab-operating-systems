#include "util.h"
#include "node.h"

transaction_pool pool;
static int transaction_pool_size = 0;
struct timespec remaining;
struct timespec request = {SECS_TO_SLEEP, NSEC_TO_SLEEP};

int main() {
    transaction t1;
    t1.timestamp = time(NULL);
    t1.sender = 111111;
    t1.receiver = 222222;
    t1.amount = 20;
    t1.reward = 5;
    t1.status = PROCESSING;

    transaction t2;
    t2.timestamp = time(NULL);
    t2.sender = 333333;
    t2.receiver = 444444;
    t2.amount = 60;
    t2.reward = 15;
    t2.status = COMPLETED;

    transaction t3;
    t3.timestamp = time(NULL);
    t3.sender = 555555;
    t3.receiver = 666666;
    t3.amount = 30;
    t3.reward = 10;
    t3.status = ABORTED;

    /* add_transaction_to_pool(t1);
    add_transaction_to_pool(t2);
    add_transaction_to_pool(t3);

    print_transaction_pool();
    printf("\n");
    remove_from_transaction_pool(t3);
    print_transaction_pool(); */

    print_transaction(t1);
    execute_transaction(&t1);
    print_transaction(t1);

    return 0;
}

void print_transaction_pool() {
    int i;
    print_table_header();
    for (i = 0; i < transaction_pool_size; i++) {
        print_transaction(pool.transactions[i]);
    }
}

int add_transaction_to_pool(transaction t) {
    int added = 0;
    if (transaction_pool_size < SO_TP_SIZE) {
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

void execute_transaction(transaction *t) {
    printf("Executing transaction...\n");
    nanosleep(&request, &remaining);
    (*t).status = COMPLETED;
}

void check_transaction_in_ledger(transaction t) {

}