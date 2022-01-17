#include "../include/util.h"
#include "../include/user.h"

configuration (*config);
ledger (*master_ledger);
pid_t (*user_list);
int *block_id;
int *readers_block_id;

/* SHARED MEMORY */
int id_shm_ledger;
int id_shm_user_list;
int id_shm_block_id;
int id_shm_readers_block_id;

/* MESSAGE QUEUE */
int id_msg_node_user;
int id_msg_user_node;

/* SEMAPHORE*/
int id_sem_init;
int id_sem_writers_block_id;
int id_sem_readers_block_id;

/* MESSAGE DATA STRUCTURE */
user_node_message user_node_msg;
user_node_message node_user_msg;

/* INTERNAL VARIABLES */
transaction (*processing_transactions);
int user_index;
int so_budget_init;
int so_retry;
int so_min_trans_gen_nsec;
int so_max_trans_gen_nsec;
int so_users_num;

int dying = 0;
int n_processing_transactions = 0;
int last_block_checked = 0;

/**
 * USER PROCESS
 * 1) Calculate balance from ledger
 * 2) Extract random user
 * 3) Calculate user revenue (amount)
 * 4) Calculate node revenue (reward)
 * 5) Send transaction
 */
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
    user_index = atoi(argv[1]);

    so_budget_init = atoi(argv[2]);
    so_retry = atoi(argv[3]);
    so_min_trans_gen_nsec = atoi(argv[4]);
    so_max_trans_gen_nsec = atoi(argv[5]);
    so_users_num = atoi(argv[6]);

    /* SHARED MEMORY ATTACHING */
    id_shm_ledger = atoi(argv[7]);
    master_ledger = shmat(id_shm_ledger, NULL, 0);
    EXIT_ON_ERROR

    id_shm_user_list = atoi(argv[8]);
    user_list = shmat(id_shm_user_list, NULL, 0);
    EXIT_ON_ERROR

    id_shm_block_id = atoi(argv[9]);
    block_id = shmat(id_shm_block_id, NULL, 0);
    EXIT_ON_ERROR

    id_shm_readers_block_id = atoi(argv[10]);
    readers_block_id = shmat(id_shm_readers_block_id, NULL, 0);
    EXIT_ON_ERROR

    /* MESSAGE QUEUE ATTACHING */
    id_msg_node_user = atoi(argv[11]);
    id_msg_user_node = atoi(argv[12]);

    /* SEMAPHORE CREATION */
    id_sem_init = atoi(argv[13]);
    id_sem_writers_block_id = atoi(argv[14]);
    id_sem_readers_block_id = atoi(argv[15]);

    wait_for_master(id_sem_init);

    while (1) {
        if (msgrcv(id_msg_node_user, &user_node_msg, sizeof(user_node_msg), getpid(), IPC_NOWAIT) != -1) {
            for (i = 0; i < n_processing_transactions; i++) {
                if (equal_transaction(user_node_msg.t, processing_transactions[i])) {
                    so_budget_init += user_node_msg.t.amount;
                    so_budget_init += user_node_msg.t.reward;
                    remove_from_processing_list(i);
                    printf("Here\n");
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

    lock(id_sem_writers_block_id);
    tmp_block_id = *block_id;
    unlock(id_sem_writers_block_id);

    /*if (last_block_checked != tmp_block_id) {*/
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

    last_block_checked = tmp_block_id;

    for (y = 0; y < n_processing_transactions; y++) {
        balance -= processing_transactions[y].amount + processing_transactions[y].reward;
    }

    /*}*/
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
    toNode = (random * 20) / 100;
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
        user_node_msg.mtype = 1;
        user_node_msg.t = transaction;
        if ((msgsnd(id_msg_user_node, &user_node_msg, sizeof(user_node_message), 0)) < 0) {
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

void print_processing_list() {
    int i;
    print_table_header();
    for (i = 0; i < n_processing_transactions; i++) {
        print_transaction(processing_transactions[i]);
    }
}

void die() {
    int i;
    int found = 0;
    for (i = 0; i < so_users_num && !found; i++) {
        if (user_list[i] == getpid()) {
            found = 1;
            raise(SIGINT);
        }
    }
}

void handler(int signal) {
    transaction t;
    switch (signal) {
        case SIGINT:
            kill(getppid(), SIGUSR1);
            exit(0);

        case SIGTERM:
            kill(getppid(), SIGUSR1);
            exit(0);

        case SIGQUIT:
            kill(getppid(), SIGUSR1);
            exit(0);

        case SIGUSR1:
            execute_transaction();
            break;

        default:
            break;
    }
}