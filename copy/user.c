#include "util.h"
#include "user.h"

configuration (*config);
ledger (*master_ledger);
node_information (*node_list);
user_information (*user_list);
int *last_block_id;

/* SHARED MEMORY */
int id_shm_configuration;
int id_shm_ledger;
int id_shm_node_list;
int id_shm_user_list;
int id_shm_last_block_id;

/* MESSAGE QUEUE */
int id_msg_node_user;
int id_msg_user_node;

/* SEMAPHORE*/
int id_sem_init;
int id_sem_writers;

/* MESSAGE DATA STRUCTURE */
user_node_message user_node_msg;
user_master_message user_master_msg;

/* INTERNAL VARIABLES */
transaction (*processing_transactions);
transaction (*completed_transactions);
int user_index;
int balance = 0;
int next_block_to_check = 0;
int dying = 0;
int n_processing_transactions = 0;
int n_completed_transactions = 0;

/**
 * USER PROCESS
 * 1) Calculate balance from ledger
 * 2) Extract random user
 * 3) Calculate user revenue (amount)
 * 4) Calculate node revenue (reward)
 * 5) Send transaction
 */
int main(int argc, char *argv[]) {
    int lower;
    int upper;
    long random;
    struct sigaction sa;
    struct timespec interval;
    transaction transaction;

    bzero(&sa, sizeof(sa));
    sa.sa_handler = handler;
    sigaction(SIGINT, &sa, 0);
    sigaction(SIGTERM, &sa, 0);
    sigaction(SIGQUIT, &sa, 0);

    /* SHARED MEMORY CREATION */
    user_index = atoi(argv[1]);

    /* SHARED MEMORY ATTACHING */
    id_shm_configuration = atoi(argv[2]);
    config = shmat(id_shm_configuration, NULL, 0);
    EXIT_ON_ERROR

            id_shm_ledger = atoi(argv[3]);
    master_ledger = shmat(id_shm_ledger, NULL, 0);
    EXIT_ON_ERROR

            id_shm_node_list = atoi(argv[4]);
    node_list = shmat(id_shm_node_list, NULL, 0);
    EXIT_ON_ERROR

            id_shm_user_list = atoi(argv[5]);
    node_list = shmat(id_shm_user_list, NULL, 0);
    EXIT_ON_ERROR

            id_shm_last_block_id = atoi(argv[6]);
    last_block_id = shmat(id_shm_last_block_id, NULL, 0);
    EXIT_ON_ERROR

    /* MESSAGE QUEUE ATTACHING */
    id_msg_node_user = atoi(argv[7]);
    id_msg_user_node = atoi(argv[8]);

    /* SEMAPHORE CREATION */
    id_sem_init = atoi(argv[9]);
    id_sem_writers = atoi(argv[10]);

    balance = config->SO_BUDGET_INIT;
    synchronize_resources(id_sem_init);
    /* while (1) {
        if (msgrcv(id_message_queue_node_user, &user_node_msg, sizeof(user_node_msg) - sizeof(long), 0, 0) < 0) {
            add_to_processing_list(user_node_msg.t);
        }

        calculate_balance();
        printf("Balance = %d\n", balance);
        if(balance < 0) {
            raise(SIGKILL);
        }
        if (balance >= 2) {
            transaction = new_transaction();
            print_transaction(transaction);
            printf("processing list\n");
            print_processing_list();
            user_node_msg.mtype = 0;
            user_node_msg.t = transaction;
            if ((msgsnd(id_message_queue_node_user, &user_node_msg, sizeof(user_node_msg) - sizeof(long), 0)) < 0) {
                dying++;
                if (dying == config->SO_RETRY) {
                    update_info(atoi(argv[1]));
                    kill(getppid(), SIGUSR1);
                }
            }

            lower = config->SO_MIN_TRANS_GEN_NSEC;
            upper = config->SO_MAX_TRANS_GEN_NSEC;
            random = (rand() % (upper - lower + 1)) + lower;
            interval.tv_sec = 0;
            interval.tv_nsec = random;
            nanosleep(&interval, NULL);
        }

        update_info(atoi(argv[1]));
    } */

    /* if (msgrcv(id_message_queue_node_user, &user_node_msg, sizeof(user_node_msg), 0, 0) < 0) {
        add_to_processing_list(user_node_msg.t);
    } */

    calculate_balance();
    if (balance >= 2) {
        transaction = new_transaction();
        user_node_msg.mtype = get_random_node();
        user_node_msg.t = transaction;
        if ((msgsnd(id_msg_user_node, &user_node_msg, sizeof(user_node_message), 0)) < 0) {
            dying++;
            if (dying == config->SO_RETRY) {
                update_info(atoi(argv[1]));
                kill(getppid(), SIGUSR1);
            }
        }

        lower = config->SO_MIN_TRANS_GEN_NSEC;
        upper = config->SO_MAX_TRANS_GEN_NSEC;
        random = (rand() % (upper - lower + 1)) + lower;
        interval.tv_sec = 0;
        interval.tv_nsec = random;
        nanosleep(&interval, NULL);
    }

    update_info();

    return 0;
}

transaction new_transaction() {
    transaction transaction;
    int toNode;
    int toUser;
    int random;
    int lower = 2;
    int upper = balance;
    struct timespec tp;

    clock_gettime(CLOCK_REALTIME, &tp);
    srand(tp.tv_sec);
    random = (rand() % (upper - lower + 1)) + lower;
    toNode = (random * 20) / 100;
    toUser = random - toNode;

    clock_gettime(CLOCK_REALTIME, &tp);
    transaction.timestamp = tp.tv_sec;
    transaction.sender = getpid();
    transaction.receiver = get_random_user();
    transaction.amount = toUser;
    transaction.reward = toNode;

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
    int random;
    int lower = 0;
    int upper = config->SO_USERS_NUM;
    struct timespec tp;
    struct timespec interval;
    interval.tv_sec = 1;

    clock_gettime(CLOCK_REALTIME, &tp);
    srand(tp.tv_sec);
    random = (rand() % (upper - lower)) + lower;
    nanosleep(&interval, NULL);

    if (user_list[random].pid == getpid()) {
        if (random == config->SO_USERS_NUM - 1) {
            random--;
        } else if (random == 0) {
            random++;
        }
    }

    return user_list[random].pid;
}

pid_t get_random_node() {
    int random;
    int lower = 0;
    int upper = config->SO_NODES_NUM;
    struct timespec tp;

    clock_gettime(CLOCK_REALTIME, &tp);
    srand(tp.tv_sec);
    random = (rand() % (upper - lower)) + lower;

    return node_list[random].pid;
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
    switch (signal) {
        case SIGINT:
            printf("User received SIGINT\n");
            kill(getppid(), SIGUSR1);
            break;

        case SIGTERM:
            printf("User received SIGTERM\n");
            kill(getppid(), SIGUSR1);
            break;

        case SIGQUIT:
            printf("User received SIGQUIT\n");
            kill(getppid(), SIGUSR1);
            break;

        default:
            break;
    }
}

void update_info() {
    printf("Last block id user = %d\n", *last_block_id);
    user_list[user_index].balance = calculate_balance();
}