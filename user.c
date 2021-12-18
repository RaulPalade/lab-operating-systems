#include "util.h"
#include "user.h"

ledger (*master_ledger);
transaction (*processing_transactions);
transaction (*completed_transactions);

static int ledger_size = 0;
static int transaction_pool_size = 0;
static int balance = 100;
static int block_id = 0;
static int next_block_to_check = 0;


static int n_processing_transactions = 0;
static int n_completed_transactions = 0;

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
    int y;

    for (i = next_block_to_check; i < ledger_size; i++) {
        for (j = 0; j < SO_BLOCK_SIZE; j++) {
            if ((*master_ledger).blocks[i].transactions[j].sender == getpid()) {
                balance -= ((*master_ledger).blocks[i].transactions[j].amount +
                            (*master_ledger).blocks[i].transactions[j].reward);
            }

            if ((*master_ledger).blocks[i].transactions[j].receiver == getpid()) {
                balance += (*master_ledger).blocks[i].transactions[j].amount;
            }

            for (y = 0; y < n_processing_transactions; y++) {
                if (equal_transaction((*master_ledger).blocks[i].transactions[j], processing_transactions[y])) {
                    add_to_completed_list(processing_transactions[y]);
                    remove_from_processing_list(y);
                }
            }
        }
    }
    next_block_to_check = ledger_size;

    for (y = 0; y < n_processing_transactions; y++) {
        balance -= processing_transactions[y].amount + processing_transactions[y].reward;
    }

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

int remove_from_processing_list(int position) {
    int removed = 0;
    int i;
    for (i = position; i < n_processing_transactions; i++) {
        processing_transactions[i] = processing_transactions[i + 1];
    }
    n_processing_transactions--;
    removed = 1;

    return removed;
}

int add_to_processing_list(transaction t) {
    processing_transactions = realloc(processing_transactions, (n_processing_transactions + 1) * sizeof(transaction));
    processing_transactions[n_processing_transactions] = t;
    n_processing_transactions++;
}

int add_to_completed_list(transaction t) {
    t.status = COMPLETED;
    completed_transactions = realloc(completed_transactions, (n_completed_transactions + 1) * sizeof(transaction));
    completed_transactions[n_completed_transactions] = t;
    n_completed_transactions++;
}

void print_processing_list() {
    int i;
    print_table_header();
    for (i = 0; i < n_processing_transactions; i++) {
        print_transaction(processing_transactions[i]);
    }
}

void print_completed_list() {
    int i;
    print_table_header();
    for (i = 0; i < n_completed_transactions; i++) {
        print_transaction(completed_transactions[i]);
    }
}