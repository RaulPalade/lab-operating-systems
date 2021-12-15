#include "util.h"
#include "user.h"

ledger (*master_ledger);

static int ledger_size = 0;
static int transaction_pool_size = 0;
static int balance = 100;
static int block_id = 0;
static int next_block_to_check = 0;


/**
 * USER PROCESS
 * 1) Calculate balance from ledger
 * 2) Extract random user
 * 3) Calculate user revenue (amount)
 * 4) Calculate node revenue (reward)
 * 5) Send transaction
 */
int main() {
    return 0;
}

transaction new_transaction(pid_t receiver, int amount, int reward) {
    transaction transaction;
    struct timespec interval;
    int toNode;
    int toUser;
    int random;
    int lower = 2;
    int upper = balance;
    interval.tv_sec = 1;
    interval.tv_nsec = 0;
    srand(time(NULL));
    random = (rand() % (upper - lower + 1)) + lower;
    toNode = (random * 20) / 100;
    toUser = random - toNode;

    transaction.timestamp = time(NULL);
    transaction.sender = getpid();
    transaction.receiver = receiver;
    transaction.amount = toUser;
    transaction.reward = toNode;
    transaction.status = PROCESSING;
    nanosleep(&interval, NULL);

    return transaction;
}

int calculate_balance() {
    int i;
    int j;

    for (i = next_block_to_check; i < ledger_size; i++) {
        for (j = 0; j < SO_BLOCK_SIZE; j++) {
            if ((*master_ledger).blocks[i].transactions[j].sender == getpid()) {
                balance -= ((*master_ledger).blocks[i].transactions[j].amount +
                            (*master_ledger).blocks[i].transactions[j].reward);
            }

            if ((*master_ledger).blocks[i].transactions[j].receiver == getpid()) {
                balance += (*master_ledger).blocks[i].transactions[j].amount;
            }
        }
    }
    next_block_to_check = ledger_size;

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