#include "../include/util.h"
#include "../include/user.h"

/* SHARED MEMORY STRUCTURES */
ledger (*master_ledger);
pid_t (*user_list);
int *block_id;

/* MESSAGE QUEUE DATA STRUCTURE */
tx_message tx_user_node;
tx_message tx_node_user;

/* SHARED MEMORY ID */
int id_shm_ledger;
int id_shm_user_list;
int id_shm_block_id;

/* MESSAGE QUEUES ID */
int id_msg_tx_node_user;
int id_msg_tx_user_node;

/* SEMAPHORE ID */
int id_sem_init;
int id_sem_block_id;
int id_sem_readers_block_id;

/* CONFIGURATION VALUES */
int so_budget_init;
int so_retry;
int so_min_trans_gen_nsec;
int so_max_trans_gen_nsec;
int so_users_num;
int so_reward;

/* INTERNAL VARIABLES */
transaction (*processing_transactions);
int dying = 0;
int n_processing_transactions = 0;

int main(int argc, char *argv[]) {
    int i;
    struct sigaction sa;
    transaction transaction;

    processing_transactions = malloc(sizeof(transaction));

    bzero(&sa, sizeof(sa));
    sa.sa_handler = handler;
    sigaction(SIGINT, &sa, 0);
    sigaction(SIGTERM, &sa, 0);
    sigaction(SIGQUIT, &sa, 0);
    sigaction(SIGUSR1, &sa, 0);

    /* SHARED MEMORY CREATION */
    so_budget_init = atoi(argv[1]);
    so_retry = atoi(argv[2]);
    so_min_trans_gen_nsec = atoi(argv[3]);
    so_max_trans_gen_nsec = atoi(argv[4]);
    so_users_num = atoi(argv[5]);
    so_reward = atoi(argv[6]);

    /* SHARED MEMORY ATTACHING */
    id_shm_ledger = atoi(argv[7]);
    master_ledger = shmat(id_shm_ledger, NULL, 0);
    TEST_ERROR

    id_shm_user_list = atoi(argv[8]);
    user_list = shmat(id_shm_user_list, NULL, 0);
    TEST_ERROR

    id_shm_block_id = atoi(argv[9]);
    block_id = shmat(id_shm_block_id, NULL, 0);
    TEST_ERROR

    /* MESSAGE QUEUE ATTACHING */
    id_msg_tx_node_user = atoi(argv[10]);
    id_msg_tx_user_node = atoi(argv[11]);

    /* SEMAPHORE CREATION */
    id_sem_init = atoi(argv[12]);
    id_sem_block_id = atoi(argv[13]);

    wait_for_master(id_sem_init);

    while (1) {
        if (msgrcv(id_msg_tx_node_user, &tx_user_node, sizeof(tx_message), getpid(), IPC_NOWAIT) != -1) {
            for (i = 0; i < n_processing_transactions; i++) {
                if (equal_transaction(tx_user_node.t, processing_transactions[i])) {
                    so_budget_init += tx_user_node.t.amount;
                    so_budget_init += tx_user_node.t.reward;
                    remove_from_processing_list(i);
                }
            }
        }

        execute_transaction();
    }
}

int calculate_balance() {
    int balance = so_budget_init;
    int i;
    int j;
    int y;
    int equal = 0;
    int tmp_block_id;

    lock(id_sem_block_id);
    tmp_block_id = *block_id;
    unlock(id_sem_block_id);

    for (i = 0; i < tmp_block_id; i++) {
        for (j = 0; j < SO_BLOCK_SIZE - 2; j++) {
            if (master_ledger->blocks[i].transactions[j].sender == getpid()) {
                balance -= master_ledger->blocks[i].transactions[j].amount;
                balance -= master_ledger->blocks[i].transactions[j].reward;
            } else if (master_ledger->blocks[i].transactions[j].receiver == getpid()) {
                balance += master_ledger->blocks[i].transactions[j].amount;
            }

            for (y = 0; y < n_processing_transactions && !equal; y++) {
                if (equal_transaction(master_ledger->blocks[i].transactions[j], processing_transactions[y])) {
                    equal = 1;
                    remove_from_processing_list(y);
                }
            }
        }
    }

    for (y = 0; y < n_processing_transactions; y++) {
        balance -= processing_transactions[y].amount + processing_transactions[y].reward;
    }

    return balance;
}

transaction new_transaction() {
    transaction transaction;
    int toNode;
    int toUser;
    int random;
    int lower = 2;
    int upper = calculate_balance();
    struct timespec tp;

    clock_gettime(CLOCK_REALTIME, &tp);
    srand(tp.tv_nsec);
    random = (rand() % (upper - lower + 1)) + lower;
    toNode = ceil((random * so_reward) / 100.00);
    toUser = random - toNode;

    transaction.timestamp = get_timestamp_millis();
    transaction.sender = getpid();
    transaction.receiver = get_random_user();
    transaction.amount = toUser;
    transaction.reward = toNode;

    return transaction;
}

void execute_transaction() {
    int lower;
    int upper;
    int random;
    struct timespec request, remaining;
    transaction transaction;
    int balance = calculate_balance();

    if (balance >= 2) {
        transaction = new_transaction();
        tx_user_node.mtype = 1;
        tx_user_node.t = transaction;
        if ((msgsnd(id_msg_tx_user_node, &tx_user_node, sizeof(tx_message), 0)) < 0) {
            dying++;
            if (dying == so_retry) {
                die();
            }
        } else {
            add_to_processing_list(transaction);
        }

        lower = so_min_trans_gen_nsec;
        upper = so_max_trans_gen_nsec;
        random = (rand() % (upper - lower + 1)) + lower;
        request.tv_sec = 0;
        request.tv_nsec = random;
        nanosleep(&request, &remaining);
    }
}

void add_to_processing_list(transaction t) {
    processing_transactions = realloc(processing_transactions, (n_processing_transactions + 1) * sizeof(transaction));
    processing_transactions[n_processing_transactions] = t;
    n_processing_transactions++;
}

void remove_from_processing_list(int position) {
    int i;
    for (i = position; i < n_processing_transactions; i++) {
        processing_transactions[i] = processing_transactions[i + 1];
    }
    n_processing_transactions--;
    processing_transactions = realloc(processing_transactions, (n_processing_transactions) * sizeof(transaction));
}

pid_t get_random_user() {
    int random;
    int lower = 0;
    int upper = so_users_num;
    struct timespec tp;

    clock_gettime(CLOCK_REALTIME, &tp);
    srand(tp.tv_nsec);
    random = (rand() % (upper - lower)) + lower;

    while (user_list[random] == getpid() || user_list[random] == -1) {
        if (random == so_users_num - 1) {
            random--;
        } else {
            random++;
        }
    }

    return user_list[random];
}

void die() {
    int i;
    int found = 0;
    for (i = 0; i < so_users_num && !found; i++) {
        if (user_list[i] == getpid()) {
            user_list[i] = -1;
            found = 1;
            raise(SIGINT);
        }
    }
}

void print_processing_list() {
    int i;
    print_table_header();
    for (i = 0; i < n_processing_transactions; i++) {
        print_transaction(processing_transactions[i]);
    }
}

void handler(int signal) {
    switch (signal) {
        case SIGINT:
            shmdt(master_ledger);
            shmdt(user_list);
            shmdt(block_id);
            kill(getppid(), SIGUSR1);
            exit(0);

        case SIGTERM:
            shmdt(master_ledger);
            shmdt(user_list);
            shmdt(block_id);
            kill(getppid(), SIGUSR1);
            exit(0);

        case SIGQUIT:
            shmdt(master_ledger);
            shmdt(user_list);
            shmdt(block_id);
            kill(getppid(), SIGUSR1);
            exit(0);

        case SIGUSR1:
            execute_transaction();
            break;

        default:
            break;
    }
}