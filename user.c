#include "util.h"
#include "user.h"

configuration (*config);
ledger (*master_ledger);
transaction (*processing_transactions);
transaction (*completed_transactions);

user_information (*user_info);
user_information (*node_info);

static int balance = 0;
static int next_block_to_check = 0;
static int dying = 0;

static int n_processing_transactions = 0;
static int n_completed_transactions = 0;

int *last_block_id;

int id_shared_memory_ledger;
int id_shared_memory_configuration;
int id_shared_memory_users;
int id_shared_memory_nodes;
int id_shared_memory_last_block_id;

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
    int i;
    key_t key;
    struct sigaction sa;
    transaction transaction;
    struct timespec interval;
    int lower;
    int upper;
    long random;
    struct msqid_ds buf;

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
        EXIT_ON_ERROR
        raise(SIGQUIT);
    }

    if ((id_shared_memory_ledger = shmget(key, 0, 0666)) < 0) {
        EXIT_ON_ERROR
        raise(SIGQUIT);
    }

    if ((void *) (master_ledger = shmat(id_shared_memory_ledger, NULL, 0)) < (void *) 0) {
        EXIT_ON_ERROR
        raise(SIGQUIT);
    }

    if ((key = ftok("./makefile", 'x')) < 0) {
        EXIT_ON_ERROR
        raise(SIGQUIT);
    }

    if ((id_shared_memory_configuration = shmget(key, 0, 0666)) < 0) {
        EXIT_ON_ERROR
        raise(SIGQUIT);
    }

    if ((void *) (config = shmat(id_shared_memory_configuration, NULL, 0)) < (void *) 0) {
        EXIT_ON_ERROR
        raise(SIGQUIT);
    }

    if ((key = ftok("./makefile", 'y')) < 0) {
        EXIT_ON_ERROR
        raise(SIGQUIT);
    }

    if ((id_shared_memory_last_block_id = shmget(key, 0, 0666)) < 0) {
        EXIT_ON_ERROR
        raise(SIGQUIT);
    }

    if ((void *) (last_block_id = shmat(id_shared_memory_last_block_id, NULL, 0)) < (void *) 0) {
        EXIT_ON_ERROR
        raise(SIGQUIT);
    }

    if ((key = ftok("./makefile", 'z')) < 0) {
        EXIT_ON_ERROR
        raise(SIGQUIT);
    }

    if ((id_shared_memory_users = shmget(key, 0, 0666)) < 0) {
        EXIT_ON_ERROR
        raise(SIGQUIT);
    }

    if ((void *) (user_info = shmat(id_shared_memory_users, NULL, 0)) < (void *) 0) {
        EXIT_ON_ERROR
        raise(SIGQUIT);
    }

    if ((key = ftok("./makefile", 'w')) < 0) {
        EXIT_ON_ERROR
        raise(SIGQUIT);
    }

    if ((id_shared_memory_nodes = shmget(key, 0, 0666)) < 0) {
        EXIT_ON_ERROR
        raise(SIGQUIT);
    }

    if ((void *) (node_info = shmat(id_shared_memory_nodes, NULL, 0)) < (void *) 0) {
        EXIT_ON_ERROR
        raise(SIGQUIT);
    }

    /* MESSAGE QUEUE CONNECTION */
    if ((key = ftok("./makefile", 'd')) < 0) {
        EXIT_ON_ERROR
        raise(SIGQUIT);
    }
    id_message_queue_master_user = msgget(key, 0666);

    if ((key = ftok("./makefile", 'e')) < 0) {
        EXIT_ON_ERROR
        raise(SIGQUIT);
    }
    id_message_queue_node_user = msgget(key, 0666);

    /* SEMAPHORE CONNECTION */
    if ((key = ftok("./makefile", 'f')) < 0) {
        EXIT_ON_ERROR
        raise(SIGQUIT);
    }
    id_semaphore_init = semget(key, 0, 0666);

    if ((key = ftok("./makefile", 'g')) < 0) {
        EXIT_ON_ERROR
        raise(SIGQUIT);
    }
    id_semaphore_writers = semget(key, 0, 0666);

    if ((key = ftok("./makefile", 'h')) < 0) {
        EXIT_ON_ERROR
        raise(SIGQUIT);
    }
    id_semaphore_mutex = semget(key, 0, 0666);

    balance = config->SO_BUDGET_INIT;
    synchronize_resources(id_semaphore_init);
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

    update_info(atoi(argv[1]));
    /* if (msgrcv(id_message_queue_node_user, &user_node_msg, sizeof(user_node_msg), 0, 0) < 0) {
        add_to_processing_list(user_node_msg.t);
    } */

    calculate_balance();
    if (balance >= 2) {
        transaction = new_transaction();
        user_node_msg.mtype = get_random_node();
        user_node_msg.t = transaction;
        print_table_header();
        print_transaction(user_node_msg.t);
        if ((msgsnd(id_message_queue_node_user, &user_node_msg, sizeof(user_node_message), 0)) < 0) {
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
                printf("no\n");
                balance -= ((*master_ledger).blocks[i].transactions[j].amount +
                            (*master_ledger).blocks[i].transactions[j].reward);
            }

            if ((*master_ledger).blocks[i].transactions[j].receiver == getpid()) {
                printf("yes\n");
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

    if (user_info[random].pid == getpid()) {
        if(random == config->SO_USERS_NUM) {
            random--;
        }
        if(random == 0) {
            random++;
        }
    }

    return user_info[random].pid;
}

pid_t get_random_node() {
    int random;
    int lower = 0;
    int upper = config->SO_NODES_NUM;
    struct timespec tp;

    clock_gettime(CLOCK_REALTIME, &tp);
    srand(tp.tv_sec);
    random = (rand() % (upper - lower)) + lower;

    return node_info[random].pid;
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
    user_info[index].balance = calculate_balance();
}