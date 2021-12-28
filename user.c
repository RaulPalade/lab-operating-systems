#include "util.h"
#include "user.h"

configuration (*config);
ledger (*master_ledger);
transaction (*processing_transactions);
transaction (*completed_transactions);

user_information (*user_info);

static int ledger_size = 0;
static int transaction_pool_size = 0;
static int balance = 100;
static int block_id = 0;
static int next_block_to_check = 0;

static int n_processing_transactions = 0;
static int n_completed_transactions = 0;

int *readers;

int id_shared_memory_ledger;
int id_shared_memory_configuration;
int id_shared_memory_readers;

int id_shared_memory_users;

int id_message_queue_master_user;
int id_message_queue_node_user;

int id_semaphore_init;
int id_semaphore_writers;
int id_semaphore_mutex;

user_node_message user_node_msg;
user_master_message user_master_msg;

/**
 * USER PROCESS
 * 1) Calculate balance from ledger
 * 2) Extract random user
 * 3) Calculate user revenue (amount)
 * 4) Calculate node revenue (reward)
 * 5) Send transaction
 */
int main() {
    key_t key;
    struct sigaction sa;
    transaction transaction;

    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handler;

    sigaction(SIGINT, &sa, 0);
    sigaction(SIGALRM, &sa, 0);
    sigaction(SIGQUIT, &sa, 0);
    sigaction(SIGUSR1, &sa, 0);
    sigaction(SIGUSR2, &sa, 0);
    sigaction(SIGTSTP, &sa, 0);

    /* SHARED MEMORY CONNECTION */
    if ((key = ftok("./makefile", 'a')) < 0) {
        raise(SIGQUIT);
    }

    if ((id_shared_memory_ledger = shmget(key, 0, 0666)) < 0) {
        raise(SIGQUIT);
    }

    if ((void *) (master_ledger = shmat(id_shared_memory_ledger, NULL, 0)) < (void *) 0) {
        raise(SIGQUIT);
    }

    if ((key = ftok("./makefile", 'x')) < 0) {
        raise(SIGQUIT);
    }

    if ((id_shared_memory_configuration = shmget(key, 0, 0666)) < 0) {
        raise(SIGQUIT);
    }

    if ((void *) (config = shmat(id_shared_memory_configuration, NULL, 0)) < (void *) 0) {
        raise(SIGQUIT);
    }

    if ((key = ftok("./makefile", 'b')) < 0) {
        raise(SIGQUIT);
    }

    if ((id_shared_memory_readers = shmget(key, 0, 0666)) < 0) {
        raise(SIGQUIT);
    }

    if ((void *) (readers = shmat(id_shared_memory_readers, NULL, 0)) < (void *) 0) {
        raise(SIGQUIT);
    }

    if ((key = ftok("./makefile", 'z')) < 0) {
        raise(SIGQUIT);
    }

    if ((id_shared_memory_users = shmget(key, 0, 0666)) < 0) {
        raise(SIGQUIT);
    }

    if ((void *) (user_info = shmat(id_shared_memory_users, NULL, 0)) < (void *) 0) {
        raise(SIGQUIT);
    }

    /* MESSAGE QUEUE CONNECTION */
    if ((key = ftok("./makefile", 'd')) < 0) {
        raise(SIGQUIT);
    }
    id_message_queue_master_user = msgget(key, 0666);


    if ((key = ftok("./makefile", 'e')) < 0) {
        raise(SIGQUIT);
    }
    id_message_queue_node_user = msgget(key, 0666);

    /* SEMAPHORE CONNECTION */
    if ((key = ftok("./makefile", 'f')) < 0) {
        raise(SIGQUIT);
    }
    id_semaphore_init = semget(key, 0, 0666);

    if ((key = ftok("./makefile", 'g')) < 0) {
        raise(SIGQUIT);
    }
    id_semaphore_writers = semget(key, 0, 0666);

    if ((key = ftok("./makefile", 'h')) < 0) {
        raise(SIGQUIT);
    }
    id_semaphore_mutex = semget(key, 0, 0666);

    synchronize_resources(id_semaphore_init);
    while (1) {
        if (balance >= 2) {
            user_node_msg.t = new_transaction();
        }
    }

    user_master_msg.balance = balance;
    msgsnd(id_message_queue_master_user, &user_master_msg, sizeof(int) - sizeof(long), IPC_NOWAIT);
}

transaction new_transaction() {
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
    transaction.receiver = 999999; /* EXtract random user */
    transaction.amount = toUser;
    transaction.reward = toNode;
    nanosleep(&interval, NULL);

    return transaction;
}

int calculate_balance() {
    int i;
    int j;
    int y;

    /* read operation */
    /* ledger_size to be updated with last_block_written */
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

int remove_from_processing_list(int position) {
    int removed = 0;
    int i;
    for (i = position; i < n_processing_transactions; i++) {
        processing_transactions[i] = processing_transactions[i + 1];
    }
    n_processing_transactions--;
    processing_transactions = realloc(processing_transactions, (n_processing_transactions) * sizeof(transaction));
    removed = 1;

    return removed;
}

int add_to_processing_list(transaction t) {
    processing_transactions = realloc(processing_transactions, (n_processing_transactions + 1) * sizeof(transaction));
    processing_transactions[n_processing_transactions] = t;
    n_processing_transactions++;
}

int add_to_completed_list(transaction t) {
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

void handler(int signal) {

}

void update_info(user_information *user) {
    
}