#include "util.h"
#include "user.h"

configuration (*config);
ledger (*master_ledger);
transaction (*processing_transactions);
transaction (*completed_transactions);

user_information (*user_info);

static int balance = 100;
static int next_block_to_check = 0;

static int n_processing_transactions = 0;
static int n_completed_transactions = 0;

int *last_block_id;     /* Used to read by user till this block */

int id_shared_memory_ledger;
int id_shared_memory_configuration;
int id_shared_memory_users;
int id_shared_memory_last_block_id;          /* Used to keep trace of block_id for next block and to read operations */

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
int main(int argc, char *argv[]) {
    key_t key;
    struct sigaction sa;
    transaction transaction;
    int index;

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

    if ((key = ftok("./makefile", 'y')) < 0) {
        raise(SIGQUIT);
    }

    if ((id_shared_memory_last_block_id = shmget(key, 0, 0666)) < 0) {
        raise(SIGQUIT);
    }

    if ((void *) (last_block_id = shmat(id_shared_memory_last_block_id, NULL, 0)) < (void *) 0) {
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
        /* RICEVERE MESSAGGIO DAL NODO SE LA TRANSAZIONE E FALLITA */
        if (msgrcv(id_message_queue_node_user, &user_node_msg, sizeof(user_node_msg) - sizeof(long), 0, 0) < 0) {
            add_to_processing_list(user_node_msg.t);
        }

        /* INVIA TRANSAZIONE SE POSSIBILE */
        calculate_balance();
        if (balance >= 2) {
            transaction = new_transaction();
            add_to_processing_list(transaction);
            user_node_msg.t = transaction;
            msgsnd(id_message_queue_node_user, &user_node_msg, sizeof(user_node_msg) - sizeof(long), 0);
        }

        /* AGGIORNARE USER INFO */
        update_info(atoi(argv[1]));
    }
}

transaction new_transaction() {
    transaction transaction;
    struct timespec interval;
    int toNode;
    int toUser;
    int random;
    int lower = 2;
    int upper = balance;
    struct timespec tp;
    clockid_t clock_id;
    interval.tv_sec = 1;
    interval.tv_nsec = 0;

    clock_gettime(clock_id, &tp);
    srand(tp.tv_sec);
    random = (rand() % (upper - lower + 1)) + lower;
    toNode = (random * 20) / 100;
    toUser = random - toNode;

    transaction.timestamp = tp.tv_sec;
    transaction.sender = getpid();
    transaction.receiver = get_random_user();
    transaction.amount = toUser;
    transaction.reward = toNode;
    nanosleep(&interval, NULL);

    return transaction;
}

int calculate_balance() {
    int i;
    int j;
    int y;

    for (i = next_block_to_check; i < *last_block_id; i++) {
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
    next_block_to_check = *last_block_id;

    for (y = 0; y < n_processing_transactions; y++) {
        balance -= processing_transactions[y].amount + processing_transactions[y].reward;
    }

    return balance;
}

pid_t get_random_user() {
    pid_t user;
    int lower = 0;
    int upper = config->SO_USERS_NUM;

    struct timespec tp;
    clockid_t clock_id;

    clock_gettime(clock_id, &tp);
    srand(tp.tv_sec);
    user = (rand() % (upper - lower + 1)) + lower;

    return user == getpid() ? get_random_user() : user;
}

pid_t get_random_node() {
    pid_t node;
    int lower = 0;
    int upper = config->SO_NODES_NUM;

    struct timespec tp;
    clockid_t clock_id;

    clock_gettime(clock_id, &tp);
    srand(tp.tv_sec);
    node = (rand() % (upper - lower + 1)) + lower;

    return node;
}

void remove_from_processing_list(int position) {
    int i;
    for (i = position; i < n_processing_transactions; i++) {
        processing_transactions[i] = processing_transactions[i + 1];
    }
    n_processing_transactions--;
    processing_transactions = realloc(processing_transactions, (n_processing_transactions) * sizeof(transaction));
}

void add_to_processing_list(transaction t) {
    processing_transactions = realloc(processing_transactions, (n_processing_transactions + 1) * sizeof(transaction));
    processing_transactions[n_processing_transactions] = t;
    n_processing_transactions++;
}

void add_to_completed_list(transaction t) {
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
    printf("Signal = %d\n", signal);
}

void update_info(int index) {
    user_info[index].balance = balance;
}