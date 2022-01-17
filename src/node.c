#include "../include/util.h"
#include "../include/node.h"

ledger (*master_ledger);
int *block_id;

/* SHARED MEMORY */
int id_shm_ledger;
int id_shm_block_id;

/* MESSAGE QUEUE */
int id_msg_node_user;
int id_msg_user_node;
int id_msg_master_node_nf;
int id_msg_node_friends;
int id_msg_node_master;
int id_msg_master_node_new_friend;

/* SEMAPHORE*/
int id_sem_init;
int id_sem_writers_block_id;

int node_index;
int so_tp_size;
int so_min_trans_proc_nsec;
int so_max_trans_proc_nsec;
int so_friends_num;
int so_hops;

transaction_pool pool;
int transaction_pool_size = 0;
int balance = 0;
int new_friends = 0;

user_node_message user_node_msg;
user_node_message node_user_msg;
master_node_fl_message master_node_fl_msg;
node_friends_message node_friends_msg;
node_master_message node_master_msg;
master_node_new_friend_message master_node_new_friend_msg;
struct timeval timer;

pid_t *node_friends;

/**
 * NODE PROCESS
 * 1) Receive transaction from User                     
 * 2) Add transaction received to transaction pool      
 * 3) Add transaction to block                          
 * 4) Add reward transaction to block                   
 * 5) Execute transaction                               
 * 6) Remove transactions from transaction pool         
 */

int main(int argc, char *argv[]) {
    int i;
    int j;
    int time_to_send = TIMER_NEW_FRIEND_TRANSACTION;
    int user_receive_success;
    int friend_receive_success;
    int added_to_ledger;
    transaction *transactions;
    block block;
    struct sigaction sa;

    bzero(&sa, sizeof(sa));
    sa.sa_handler = handler;
    sigaction(SIGINT, &sa, 0);
    sigaction(SIGTERM, &sa, 0);
    sigaction(SIGQUIT, &sa, 0);

    node_index = atoi(argv[1]);

    so_tp_size = atoi(argv[2]);
    so_min_trans_proc_nsec = atoi(argv[3]);
    so_max_trans_proc_nsec = atoi(argv[4]);

    /* SHARED MEMORY ATTACHING */
    id_shm_ledger = atoi(argv[5]);
    master_ledger = shmat(id_shm_ledger, NULL, 0);
    EXIT_ON_ERROR

    id_shm_block_id = atoi(argv[6]);
    block_id = shmat(id_shm_block_id, NULL, 0);
    EXIT_ON_ERROR

    /* MESSAGE QUEUE ATTACHING */
    id_msg_node_user = atoi(argv[7]);
    id_msg_user_node = atoi(argv[8]);

    /* SEMAPHORE CREATION */
    id_sem_init = atoi(argv[9]);
    id_sem_writers_block_id = atoi(argv[10]);
    so_friends_num = atoi(argv[11]);

    /* EXTRA */
    id_msg_master_node_nf = atoi(argv[12]);
    id_msg_node_friends = atoi(argv[13]);
    so_hops = atoi(argv[14]);
    id_msg_node_master = atoi(argv[15]);
    id_msg_master_node_new_friend = atoi(argv[16]);

    pool.transactions = malloc(so_tp_size * sizeof(transaction));

    if (msgrcv(id_msg_master_node_nf, &master_node_fl_msg, sizeof(master_node_fl_message), getpid(), IPC_NOWAIT) !=
        -1) {
        node_friends = master_node_fl_msg.friends;
    }

    if (msgrcv(id_msg_node_master, &node_master_msg, sizeof(node_master_message), getpid(), IPC_NOWAIT) != -1) {
        add_to_transaction_pool(node_master_msg.t);
    }

    wait_for_master(id_sem_init);

    while (1) {
        if (msgrcv(id_msg_master_node_new_friend, &master_node_new_friend_msg, sizeof(master_node_new_friend_message),
                   getpid(),
                   IPC_NOWAIT) != -1) {
            add_new_friend(master_node_new_friend_msg.new_friend);
        }

        /* FRIEND TRANSACTION */
        if (msgrcv(id_msg_node_friends, &node_friends_msg, sizeof(node_friends_message), getpid(), 0) != -1) {
            friend_receive_success = add_to_transaction_pool(node_friends_msg.f_transaction.t);
            print_transaction(node_friends_msg.f_transaction.t);
            if (!friend_receive_success) {
                node_friends_msg.f_transaction.hops++;
                if (node_friends_msg.f_transaction.hops > so_hops) {
                    node_friends_msg.mtype = getppid();
                    msgsnd(id_msg_node_master, &node_master_msg, sizeof(node_master_message), 0);
                } else {
                    node_friends_msg.mtype = get_random_friend();
                    msgsnd(id_msg_node_friends, &node_friends_msg, sizeof(node_friends_message), 0);
                }
            }
        }


        /* USER TRANSACTION */
        if (msgrcv(id_msg_user_node, &user_node_msg, sizeof(user_node_message), 1, IPC_NOWAIT) != -1) {
            user_receive_success = add_to_transaction_pool(user_node_msg.t);
            if (!user_receive_success) {
                user_node_msg.mtype = user_node_msg.t.sender;
                msgsnd(id_msg_node_user, &user_node_msg, sizeof(user_node_message), 0);
            } else {
                if (transaction_pool_size >= 1) {
                    printf("Send new friend transaction\n");
                    time_to_send = TIMER_NEW_FRIEND_TRANSACTION;
                    node_friends_msg.mtype = get_random_friend();
                    node_friends_msg.f_transaction.hops = 0;
                    node_friends_msg.f_transaction.t = pool.transactions[0];
                }
                /*if (time_to_send == 0 && transaction_pool_size >= 1) {
                    printf("Send new friend transaction\n");
                    time_to_send = TIMER_NEW_FRIEND_TRANSACTION;
                    node_friends_msg.mtype = get_random_friend();
                    node_friends_msg.f_transaction.hops = 0;
                    node_friends_msg.f_transaction.t = pool.transactions[0];
                }
                if (transaction_pool_size >= SO_BLOCK_SIZE - 1) {
                    transactions = extract_transactions_block_from_pool();
                    block = new_block(transactions);
                    added_to_ledger = add_to_ledger(block);
                    if (added_to_ledger) {
                        for (i = 0; i < SO_BLOCK_SIZE - 1; i++) {
                            remove_from_transaction_pool(transactions[i]);
                        }
                    }
                }*/
            }
        }
        time_to_send--;
    }
}

pid_t get_random_friend() {
    pid_t friend;
    int lower = 0;
    int upper = so_friends_num;
    struct timespec tp;
    clock_gettime(CLOCK_REALTIME, &tp);
    srand(tp.tv_nsec);
    friend = (rand() % (upper - lower)) + lower;
    return friend;
}

void add_new_friend(pid_t node) {
    new_friends++;
    node_friends = realloc(node_friends, (so_friends_num + new_friends) * sizeof(pid_t));
    node_friends[so_friends_num + new_friends] = node;
}

int add_to_transaction_pool(transaction t) {
    int added = 0;
    if (transaction_pool_size < so_tp_size) {
        pool.transactions[transaction_pool_size] = t;
        transaction_pool_size++;
        added = 1;
    } else {
        printf("TP FULL\n");
    }

    return added;
}

int remove_from_transaction_pool(transaction t) {
    int removed = 0;
    int i;
    int position;
    for (i = 0; i < transaction_pool_size && !removed; i++) {
        if (pool.transactions[i].timestamp == t.timestamp && pool.transactions[i].sender == t.sender
            && pool.transactions[i].receiver == t.receiver) {
            for (position = i; position < transaction_pool_size; position++) {
                pool.transactions[position] = pool.transactions[position + 1];
            }
            transaction_pool_size--;
            removed = 1;
        }
    }

    return removed;
}

transaction *extract_transactions_block_from_pool() {
    int i;
    transaction *transactions = malloc((SO_BLOCK_SIZE - 1) * sizeof(transaction));
    for (i = 0; i < SO_BLOCK_SIZE - 1; i++) {
        transactions[i] = pool.transactions[i];
    }

    return transactions;
}

block new_block(transaction transactions[]) {
    int i;
    int total_reward = 0;
    int random;
    struct timespec interval;
    int lower = so_min_trans_proc_nsec;
    int upper = so_max_trans_proc_nsec;
    block block;
    struct timespec tp;

    clock_gettime(CLOCK_REALTIME, &tp);
    srand(tp.tv_nsec);
    random = (rand() % (upper - lower + 1)) + lower;

    interval.tv_sec = 0;
    interval.tv_nsec = random;

    for (i = 0; i < SO_BLOCK_SIZE - 1; i++) {
        block.transactions[i] = transactions[i];
        total_reward += transactions[i].reward;
    }
    block.transactions[SO_BLOCK_SIZE - 1] = new_reward_transaction(total_reward);
    nanosleep(&interval, NULL);

    return block;
}

transaction new_reward_transaction(int total_amount) {
    transaction transaction;
    transaction.timestamp = get_timestamp_millis();
    transaction.sender = SENDER_TRANSACTION_REWARD;
    transaction.receiver = getpid();
    transaction.amount = total_amount;
    transaction.reward = 0;
    return transaction;
}

int add_to_ledger(block block) {
    int added = 0;
    lock(id_sem_writers_block_id);
    if (*block_id < SO_REGISTRY_SIZE) {
        block.id = *block_id;
        master_ledger->blocks[*block_id] = block;
        (*block_id)++;
        balance += block.transactions[SO_BLOCK_SIZE - 1].amount;
        added = 1;
    } else {
        kill(getppid(), SIGUSR2);
    }
    unlock(id_sem_writers_block_id);

    return added;
}

void print_transaction_pool() {
    int i;
    print_table_header();
    for (i = 0; i < transaction_pool_size; i++) {
        print_transaction(pool.transactions[i]);
    }
}

void clean_transaction_pool() {
    /* Maybe need to release semaphores */
    int i;
    if (transaction_pool_size > 0) {
        for (i = 0; i < transaction_pool_size; i++) {
            user_node_msg.mtype = pool.transactions[i].sender;
            user_node_msg.t = pool.transactions[i];
            msgsnd(id_msg_node_user, &user_node_msg, sizeof(user_node_message), 0);
        }
    }
    transaction_pool_size = 0;
    free(pool.transactions);
}

void handler(int signal) {
    switch (signal) {
        case SIGINT:
            free(node_friends);
            clean_transaction_pool();
            kill(getppid(), SIGUSR2);
            break;

        case SIGTERM:
            free(node_friends);
            clean_transaction_pool();
            kill(getppid(), SIGUSR2);
            exit(0);

        case SIGQUIT:
            free(node_friends);
            clean_transaction_pool();
            kill(getppid(), SIGUSR2);
            break;

        default:
            break;
    }
}