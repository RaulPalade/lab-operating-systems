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
int id_shm_ledger_size;

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

    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handler;

    sigaction(SIGINT, &sa, 0);
    sigaction(SIGALRM, &sa, 0);
    sigaction(SIGQUIT, &sa, 0);
    sigaction(SIGUSR1, &sa, 0);
    sigaction(SIGUSR2, &sa, 0);
    sigaction(SIGTSTP, &sa, 0);

    /* SHARED MEMORY CREATION */
    config = new_shared_memory(PROJ_ID_SHM_CONFIGURATION, id_shm_configuration, sizeof(config));
    master_ledger = new_shared_memory(PROJ_ID_SHM_LEDGER, id_shm_ledger, sizeof(ledger));
    node_list = new_shared_memory(PROJ_ID_SHM_NODE_LIST, id_shm_node_list,
                                  sizeof(node_information) * config->SO_NODES_NUM);
    user_list = new_shared_memory(PROJ_ID_SHM_USER_LIST, id_shm_user_list,
                                  sizeof(user_information) * config->SO_USERS_NUM);
    last_block_id = new_shared_memory(PROJ_ID_SHM_LAST_BLOCK_ID, id_shm_last_block_id, sizeof(last_block_id));

    /* MESSAGE QUEUE CREATION */
    id_msg_node_user = new_message_queue(PROJ_ID_MSG_NODE_USER);
    id_msg_user_node = new_message_queue(PROJ_ID_MSG_USER_NODE);

    /* SEMAPHORE CREATION */
    id_sem_init = new_semaphore(PROJ_ID_SEM_INIT);

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

    update_info(atoi(argv[1]));

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
        if (random == config->SO_USERS_NUM) {
            random--;
        }
        if (random == 0) {
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
    /* if (signal == SIGINT) {
        printf("User received SIGINT\n");
        raise(SIGKILL);
    } */
    switch (signal)
    {
    case SIGALRM:
        raise(SIGINT);
        break;

    case SIGINT:
        printf("User received SIGINT\n");
        /* shmdt(master_ledger);
        shmdt(config);
        shmdt(last_block_id);
        shmdt(user_info); */
        raise(SIGKILL);

    case SIGUSR1:
        printf("User received SIGUSR1\n");
        break;

    case SIGTSTP:
        printf("User received SIGTSTP\n");
        /* user_request(); */
        break;
    
    default:
        break;
    }
}

void update_info(int index) {
    printf("Last block id user = %d\n", *last_block_id);
    user_list[index].balance = calculate_balance();
}