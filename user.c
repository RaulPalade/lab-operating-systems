#include "util.h"
#include "user.h"
#include <math.h>

struct timespec remaining;
struct timespec request = {SECS_TO_SLEEP, NSEC_TO_SLEEP};
ledger master_ledger;
static int ledger_size = 0;
static int block_size = 0;
static int block_id = 0;
static int balance = 100;

static int last_block_confirmed;

void print_ledger_x();

/**
 * HOW IT WORKS?
 * 1) Calculate balance from ledger
 * 2) Extract random user
 * 3) Calculate user revenue (amount)
 * 4) Calculate node revenue (reward)
 * 5) Send transaction
 */

int main() {
    block block;
    ledger master_ledger;    
    transaction sended_transactions[10];

    transaction t1 = new_transaction(111111, 100, 20);
    transaction t2 = new_transaction(222222, 100, 20);
    transaction t3 = new_transaction(222222, 100, 20);
    transaction t4 = new_transaction(111111, 100, 20);
    transaction t5 = new_transaction(111111, 100, 20);

    sended_transactions[0] = t1;

    block.id = 0;
    add_to_block(&block, t1);
    add_to_block(&block, t2);
    add_to_block(&block, t3);
    add_to_block(&block, t4);
    add_to_block(&block, t5);
    
    add_to_ledger(block);

    print_ledger_x();
}

transaction new_transaction(pid_t receiver, int amount, int reward) {
    int toNode;
    int toUser;
    int random;
    srand(time(NULL));

    int lower = 2;
    int upper = balance;
    random = (rand() % (upper - lower + 1)) + lower;
    toNode = (random * 20) / 100;
    toUser = random - toNode;
    printf("Random=%d,  ToUser=%d, ToNode=%d\n", random, toUser, toNode);
    
    transaction transaction;
    transaction.timestamp = time(NULL);
    transaction.sender = getpid();
    transaction.receiver = receiver;
    transaction.amount = toUser;
    transaction.reward = toNode;
    transaction.status = PROCESSING;
    //nanosleep(&request, &remaining);
    
    return transaction;
}

int calculate_balance() {
    int i;
    int j;

    for (i = 0; i < ledger_size; i++) {
        for (j = 0; j < block_size; j++) {
            if (master_ledger.blocks[i].transactions[j].sender == getpid()) {
                balance = balance - master_ledger.blocks[i].transactions[j].amount;
                printf("Sender with amount = %d\n", master_ledger.blocks[i].transactions[j].amount);
            }

            if (master_ledger.blocks[i].transactions[j].receiver == getpid()) {
                balance = balance + master_ledger.blocks[i].transactions[j].amount;
                printf("Receiver with amount = %d\n", master_ledger.blocks[i].transactions[j].amount);
            }
        }
    }

    printf("Balance=%d\n", balance);

    return balance;
}

pid_t get_random_user() {
    return 0;
}

pid_t get_random_node() {
    return 0;
}

int handler(int signal) {
    return 0;
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

    return added;
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

    return added;
}

void print_ledger_x() {
    int i;
    int j;
    print_table_header();
    for (i = 0; i < ledger_size; i++) {
        for (j = 0; j < block_size; j++) {
            print_transaction(master_ledger.blocks[i].transactions[j]);
        }
    }
}