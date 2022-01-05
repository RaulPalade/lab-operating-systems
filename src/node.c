#include "../include/util.h"
#include "../include/node.h"

configuration (*config);
ledger (*master_ledger);
node_information (*node_list);
user_information (*user_list);
int *last_block_id;
int *ledger_size;

/* SHARED MEMORY */
int id_shm_configuration;
int id_shm_ledger;
int id_shm_node_list;
int id_shm_last_block_id;
int id_shm_ledger_size;

/* MESSAGE QUEUE */
int id_msg_node_user;
int id_msg_user_node;

/* SEMAPHORE*/
int id_sem_init;
int id_sem_writers;

int node_index;
transaction_pool pool;
int transaction_pool_size = 0;
int balance = 0;
int next_block_to_check = 0;

user_node_message user_node_msg;
struct timeval timer;

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
    int success;
    transaction *transactions;
    block block;
    struct sigaction sa;

    bzero(&sa, sizeof(sa));
    sa.sa_handler = handler;
    sigaction(SIGINT, &sa, 0);
    sigaction(SIGTERM, &sa, 0);
    sigaction(SIGQUIT, &sa, 0);

    node_index = atoi(argv[1]);

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

    id_shm_last_block_id = atoi(argv[5]);
    last_block_id = shmat(id_shm_last_block_id, NULL, 0);
    EXIT_ON_ERROR

    id_shm_ledger_size = atoi(argv[6]);
    ledger_size = shmat(id_shm_ledger_size, NULL, 0);

    /* MESSAGE QUEUE ATTACHING */
    id_msg_node_user = atoi(argv[7]);
    id_msg_user_node = atoi(argv[8]);

    /* SEMAPHORE CREATION */
    id_sem_init = atoi(argv[9]);
    id_sem_writers = atoi(argv[10]);

    pool.transactions = malloc(config->SO_TP_SIZE * sizeof(transactions));

    printf("Ledger size = %d\n", *ledger_size);
    wait_for_master(id_sem_init);

    /* while (1) {
        if (msgrcv(id_message_queue_node_user, &user_node_msg, sizeof(transaction) - sizeof(long), 0, 0) < 0) {
            success = add_to_transaction_pool(user_node_msg.t);
            if (!success) {
                user_node_msg.mtype = getpid();
                msgsnd(id_message_queue_node_user, &user_node_msg, sizeof(transaction) - sizeof(long), 0);
            }

            if (transaction_pool_size >= SO_BLOCK_SIZE) {
                transactions = extract_transactions_block_from_pool();
                block = new_block(transactions);

                acquire_resource(id_semaphore_writers, *last_block_id);
                success = add_to_ledger(master_ledger, block);
                release_resource(id_semaphore_writers, *last_block_id);
                if (success) {
                    for (i = 0; i < SO_BLOCK_SIZE - 1; i++) {
                        remove_from_transaction_pool(transactions[i]);
                    }
                }
            }
        }
        update_info(atoi(argv[1]));
    } */

    /* msgrcv(id_message_queue_node_user, &user_node_msg, sizeof(user_node_message), getpid(), 0);
    print_transaction(user_node_msg.t);

     msgrcv(id_message_queue_node_user, &user_node_msg, sizeof(user_node_message), getpid(), 0);
    print_transaction(user_node_msg.t); */
    msgrcv(id_msg_user_node, &user_node_msg, sizeof(user_node_message), getpid(), 0);
    success = add_to_transaction_pool(user_node_msg.t);
    msgrcv(id_msg_user_node, &user_node_msg, sizeof(user_node_message), getpid(), 0);
    success = add_to_transaction_pool(user_node_msg.t);
    print_transaction_pool();
/*     if (!success) {
        user_node_msg.mtype = 2;
        msgsnd(id_message_queue_node_user, &user_node_msg, sizeof(user_node_msg), 1);
    }*/

    if (transaction_pool_size >= SO_BLOCK_SIZE - 1) {
        printf("transaction_pool_size >= SO_BLOCK_SIZE - 1\n");
        transactions = extract_transactions_block_from_pool();
        printf("Here\n");
        block = new_block(transactions);

        acquire_resource(id_sem_writers, *last_block_id);
        success = add_to_ledger(master_ledger, block);
        release_resource(id_sem_writers, *last_block_id);
        if (success) {
            for (i = 0; i < SO_BLOCK_SIZE - 1; i++) {
                remove_from_transaction_pool(transactions[i]);
            }
        } else {
            printf("Failure\n");
        }
    } else {
        printf("transaction_pool_size < SO_BLOCK_SIZE\n");
    }

    printf("Ledger size = %d\n", *ledger_size);
    update_info();

    return 0;
}

int add_to_transaction_pool(transaction t) {
    int added = 0;
    if (transaction_pool_size < config->SO_TP_SIZE - 1) {
        pool.transactions[transaction_pool_size] = t;
        transaction_pool_size++;
        added = 1;
    } else {
        printf(ANSI_COLOR_RED "Transaction pool size exceeded\n" ANSI_COLOR_RESET);
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
    int added = 0;
    int lower = 0;
    int upper = transaction_pool_size;
    int random;
    transaction *transactions = malloc((SO_BLOCK_SIZE - 1) * sizeof(transaction));
    struct timespec tp;

    clock_gettime(CLOCK_REALTIME, &tp);
    srand(tp.tv_sec);

    while (added < SO_BLOCK_SIZE - 1) {
        random = (rand() % (upper - lower)) + lower;
        if (!array_contains(transactions, pool.transactions[random])) {
            transactions[added] = pool.transactions[random];
            added++;
        }
    }

    return transactions;
}

block new_block(transaction transactions[]) {
    int i;
    int total_reward = 0;
    int random;
    struct timespec interval;
    int lower = config->SO_MIN_TRANS_PROC_NSEC;
    int upper = config->SO_MAX_TRANS_PROC_NSEC;
    block block;
    struct timespec tp;

    clock_gettime(CLOCK_REALTIME, &tp);
    srand(tp.tv_sec);
    random = (rand() % (upper - lower)) + lower;

    interval.tv_sec = 0;
    interval.tv_nsec = random;

    block.id = *(last_block_id + 1);
    for (i = 0; i < SO_BLOCK_SIZE - 1; i++) {
        block.transactions[i] = transactions[i];
        total_reward += transactions[i].reward;
    }
    block.transactions[SO_BLOCK_SIZE - 1] = new_reward_transaction(total_reward);
    nanosleep(&interval, NULL);

    return block;
}

transaction new_reward_transaction(int total_amount) {
    struct timespec tp;
    transaction transaction;

    clock_gettime(CLOCK_REALTIME, &tp);
    transaction.timestamp = tp.tv_sec;
    transaction.sender = SENDER_TRANSACTION_REWARD;
    transaction.receiver = getpid();
    transaction.amount = total_amount;
    transaction.reward = 0;

    return transaction;
}

int add_to_ledger(ledger *ledger, block block) {
    int added = 0;
    if (*ledger_size < SO_REGISTRY_SIZE) {
        (*ledger).blocks[*ledger_size] = block;
        added = 1;
        balance += block.transactions[SO_BLOCK_SIZE - 1].amount;
        (*last_block_id)++;
        (*ledger_size)++;
        printf("Ledger size = %d\n", *ledger_size);
        printf("last_block_id = %d\n", *last_block_id);
    } else {
        kill(getppid(), SIGUSR2);
        printf(ANSI_COLOR_RED "Ledger size exceeded\n" ANSI_COLOR_RESET);
    }

    print_transaction(master_ledger->blocks[0].transactions[0]);
    print_transaction(master_ledger->blocks[0].transactions[1]);
    print_transaction(master_ledger->blocks[0].transactions[2]);

    return added;
}


/* Useless, when a set of transactions are extracted, they are
   not in the ledger because when a block is added to ledger
   automatically those transactions are removed from pool */
int ledger_has_transaction(ledger *ledger, transaction t) {
    int found = 0;
    int i;
    int j;

    for (i = 0; i < *ledger_size && !found; i++) {
        for (j = 0; j < SO_BLOCK_SIZE; j++) {
            if ((*ledger).blocks[i].transactions[j].timestamp == t.timestamp &&
                (*ledger).blocks[i].transactions[j].sender == t.sender) {
                found = 1;
            }
        }
        next_block_to_check++;
    }

    return found;
}

void print_transaction_pool() {
    int i;
    printf("Printing transaction pool\n");
    print_table_header();
    for (i = 0; i < transaction_pool_size; i++) {
        print_transaction(pool.transactions[i]);
    }
}

void update_info() {
    node_list[node_index].balance = balance;
    node_list[node_index].transactions_left = transaction_pool_size;
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
            printf("Node received SIGINT\n");
            clean_transaction_pool();
            kill(getppid(), SIGUSR2);
            break;

        case SIGTERM:
            printf("Node received SIGTERM\n");
            clean_transaction_pool();
            kill(getppid(), SIGUSR2);
            break;

        case SIGQUIT:
            printf("Node received SIGQUIT\n");
            clean_transaction_pool();
            kill(getppid(), SIGUSR2);
            break;

        default:
            break;
    }
}