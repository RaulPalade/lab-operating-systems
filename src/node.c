#include "../include/util.h"
#include "../include/node.h"

ledger (*master_ledger);
int *block_id;

/* SHARED MEMORY */
int id_shm_ledger;
int id_shm_block_id;

/* MESSAGE QUEUE IDS */
int id_msg_tx_node_master;      /* USED TO SEND TRANSACTION WITH MAX HOPS FROM NODE TO MASTER */
int id_msg_tx_node_user;        /* USED TO SEND FAILURE TRANSACTION FROM NODE TO USER */
int id_msg_tx_user_node;        /* USED TO SEND NEW TRANSACTION FROM USER TO NODE */
int id_msg_tx_node_friends;     /* USED TO SEND TRANSACTION FROM NODE TO ANOTHER FRIEND NODE */
int id_msg_friend_list;         /* USED TO SEND NODE FRIEND LIST FROM MASTER TO NODE */

/* SEMAPHORE*/
int id_sem_init;
int id_sem_block_id;

int node_index;
int so_tp_size;
int so_min_trans_proc_nsec;
int so_max_trans_proc_nsec;
int so_friends_num;
int so_hops;

transaction_pool pool;
int transaction_pool_size = 0;
int balance = 0;

tx_message tx_node_master;
tx_message tx_user_node;
tx_message tx_node_user;
friend_message tx_node_friend;
friend_list_message friend_list_msg;
struct timeval timer;

pid_t *friends;

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
    pid_t new_friend = 99999;
    int i;
    int j;
    int time_to_send = TIMER_NEW_FRIEND_TRANSACTION;
    int user_receive_success;
    int added_to_tp = 0;
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

    /* CONFIG PARAMS */
    so_tp_size = atoi(argv[2]);
    so_min_trans_proc_nsec = atoi(argv[3]);
    so_max_trans_proc_nsec = atoi(argv[4]);
    so_friends_num = atoi(argv[5]);
    so_hops = atoi(argv[6]);

    /* SHARED MEMORY ATTACHING */
    id_shm_ledger = atoi(argv[7]);
    master_ledger = shmat(id_shm_ledger, NULL, 0);
    TEST_ERROR

    id_shm_block_id = atoi(argv[8]);
    block_id = shmat(id_shm_block_id, NULL, 0);
    TEST_ERROR

    /* MESSAGE QUEUE ATTACHING */
    id_msg_tx_node_master = atoi(argv[9]);
    id_msg_tx_node_user = atoi(argv[10]);
    id_msg_tx_user_node = atoi(argv[11]);
    id_msg_tx_node_friends = atoi(argv[12]);
    id_msg_friend_list = atoi(argv[13]);

    /* SEMAPHORE CREATION */
    id_sem_init = atoi(argv[14]);
    id_sem_block_id = atoi(argv[15]);

    pool.transactions = malloc(so_tp_size * sizeof(transaction));
    friends = malloc(so_friends_num * sizeof(pid_t));

    /* ADDING FRIENDS DURING NODE INITIALIZATION */
    if (msgrcv(id_msg_friend_list, &friend_list_msg, sizeof(friend_list_message), getpid(), 0) !=
        -1) {
        memcpy(friends, friend_list_msg.friends, sizeof(pid_t) * so_friends_num);
        /*for (i = 0; i < so_friends_num; i++) {
            printf("%d\n", friends[i]);
        }
        printf("\n");*/
    }

    /* ADDING FAILED TRANSACTION TO POOL AFTER CREATION FROM MASTER */
    if (msgrcv(id_msg_tx_node_master, &tx_node_master, sizeof(tx_message), getpid(), IPC_NOWAIT) != -1) {
        add_to_transaction_pool(tx_node_master.t);
        printf("%d i'm adding the tx to my pool\n", getpid());
        printf("List of friends\n");
        for (i = 0; i < so_friends_num; i++) {
            printf("%d\n", friends[i]);
        }
    }

    wait_for_master(id_sem_init);

    while (1) {
        /* WAITING NEW FRIEND FROM MASTER */
        /* CORRECT */
        if (msgrcv(id_msg_friend_list, &friend_list_msg, sizeof(friend_list_message), getpid(),
                   IPC_NOWAIT) != -1) {
            printf("%d received new friend from master with PID = %d\n", getpid(), friend_list_msg.friends[0]);
            add_new_friend(friend_list_msg.friends[0]);
        }

        /* FRIEND TRANSACTION */
        if (msgrcv(id_msg_tx_node_friends, &tx_node_friend, sizeof(friend_message), getpid(), IPC_NOWAIT) != -1) {
            /*added_to_tp = add_to_transaction_pool(tx_node_friend.f_transaction.t);*/
            /* CORRECT */
            if (!added_to_tp) {
                tx_node_friend.f_transaction.hops++;
                if (tx_node_friend.f_transaction.hops > 0) {
                    /* CORRECT */
                    tx_node_master.mtype = getppid();
                    tx_node_master.t = tx_node_friend.f_transaction.t;
                    msgsnd(id_msg_tx_node_master, &tx_node_master, sizeof(tx_message), 0);
                } else {
                    /* CORRECT */
                    tx_node_friend.mtype = get_random_friend();
                    msgsnd(id_msg_tx_node_friends, &tx_node_friend, sizeof(friend_message), 0);
                }
            }
        } else {
            /*printf("No message from friend\n");*/
        }

        /* USER TRANSACTION */
        if (msgrcv(id_msg_tx_user_node, &tx_user_node, sizeof(tx_message), 1, IPC_NOWAIT) != -1) {
            user_receive_success = add_to_transaction_pool(tx_user_node.t);
            /* CORRECT */
            if (!user_receive_success) {
                tx_node_user.mtype = tx_user_node.t.sender;
                msgsnd(id_msg_tx_node_user, &tx_node_user, sizeof(tx_message), 0);
            } else {
                if (transaction_pool_size >= 1) {
                    time_to_send = TIMER_NEW_FRIEND_TRANSACTION;
                    tx_node_friend.mtype = get_random_friend();
                    tx_node_friend.f_transaction.hops = 0;
                    tx_node_friend.f_transaction.t = pool.transactions[0];
                    remove_from_transaction_pool(tx_node_friend.f_transaction.t);
                    msgsnd(id_msg_tx_node_friends, &tx_node_friend, sizeof(friend_message), 0);
                }
                /*if (time_to_send == 0 && transaction_pool_size >= 1) {
                    printf("Send new friend transaction\n");
                    time_to_send = TIMER_NEW_FRIEND_TRANSACTION;
                    tx_node_friend.mtype = get_random_friend();
                    tx_node_friend.f_transaction.hops = 0;
                    tx_node_friend.f_transaction.t = pool.transactions[0];
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
    int random;
    int lower = 0;
    int upper = so_friends_num;
    struct timespec tp;
    clock_gettime(CLOCK_REALTIME, &tp);
    srand(tp.tv_nsec);
    random = (rand() % (upper - lower)) + lower;
    return friends[random];
}

void add_new_friend(pid_t node) {
    so_friends_num++;
    friends = realloc(friends, (so_friends_num) * sizeof(pid_t));
    friends[so_friends_num - 1] = node;
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
    lock(id_sem_block_id);
    if (*block_id < SO_REGISTRY_SIZE) {
        block.id = *block_id;
        master_ledger->blocks[*block_id] = block;
        (*block_id)++;
        balance += block.transactions[SO_BLOCK_SIZE - 1].amount;
        added = 1;
    } else {
        kill(getppid(), SIGUSR2);
    }
    unlock(id_sem_block_id);

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
            tx_user_node.mtype = pool.transactions[i].sender;
            tx_user_node.t = pool.transactions[i];
            msgsnd(id_msg_tx_node_user, &tx_user_node, sizeof(tx_message), 0);
        }
    }
    transaction_pool_size = 0;
    free(pool.transactions);
}

void handler(int signal) {
    switch (signal) {
        case SIGINT:
            clean_transaction_pool();
            kill(getppid(), SIGUSR2);
            break;

        case SIGTERM:
            clean_transaction_pool();
            kill(getppid(), SIGUSR2);
            exit(0);

        case SIGQUIT:
            clean_transaction_pool();
            kill(getppid(), SIGUSR2);
            break;

        default:
            break;
    }
}