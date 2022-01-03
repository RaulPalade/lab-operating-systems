#include "util.h"
#include "node.h"

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
    last_block_id = new_shared_memory(PROJ_ID_SHM_LAST_BLOCK_ID, id_shm_last_block_id, sizeof(last_block_id));
    ledger_size = new_shared_memory(PROJ_ID_SHM_LEDGER_SIZE, id_shm_ledger, sizeof(ledger_size));

    /* MESSAGE QUEUE CREATION */
    id_msg_node_user = new_message_queue(PROJ_ID_MSG_NODE_USER);
    id_msg_user_node = new_message_queue(PROJ_ID_MSG_USER_NODE);

    /* SEMAPHORE CREATION */
    id_sem_init = new_semaphore(PROJ_ID_SEM_INIT);
    id_sem_writers = new_semaphore(PROJ_ID_SEM_WRITERS);

    pool.transactions = malloc(config->SO_TP_SIZE * sizeof(transactions));

    printf("Ledger size = %d\n", *ledger_size);
    synchronize_resources(id_sem_init);

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
/*     if (!success) {
        user_node_msg.mtype = 2;
        msgsnd(id_message_queue_node_user, &user_node_msg, sizeof(user_node_msg), 1);
    }*/

    if (transaction_pool_size >= SO_BLOCK_SIZE) {
        printf("transaction_pool_size >= SO_BLOCK_SIZE\n");
        transactions = extract_transactions_block_from_pool();
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
    update_info(atoi(argv[1]));

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

int add_to_ledger(ledger *ledger, block block) {
    int added = 0;
    if (*ledger_size < SO_REGISTRY_SIZE) {
        (*ledger).blocks[*ledger_size] = block;
        (*last_block_id)++;
        added = 1;
        balance += block.transactions[SO_BLOCK_SIZE - 1].amount;
        (*ledger_size)++;
        printf("Ls = %d\n", *ledger_size);
    } else {
        kill(getppid(), SIGUSR2);
        printf(ANSI_COLOR_RED "Ledger size exceeded\n" ANSI_COLOR_RESET);
    }

    print_transaction(master_ledger->blocks[0].transactions[0]);
    print_transaction(master_ledger->blocks[0].transactions[1]);

    return added;
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

transaction *extract_transactions_block_from_pool() {
    int confirmed = 0;
    int lower = 0;
    int upper = transaction_pool_size - 1;
    int random;
    transaction *transactions = malloc((SO_BLOCK_SIZE - 1) * sizeof(transaction));
    int numbers[SO_BLOCK_SIZE - 1];
    struct timespec tp;

    clock_gettime(CLOCK_REALTIME, &tp);
    srand(tp.tv_sec);

    while (confirmed < SO_BLOCK_SIZE - 1) {
        random = (rand() % (upper - lower)) + lower;
        if (!array_contains(numbers, random)) {
            numbers[confirmed] = random;
            transactions[confirmed] = pool.transactions[random];
            confirmed++;
        }
    }

    return transactions;
}

void print_transaction_pool() {
    int i;
    printf("Printing transaction pool\n");
    print_table_header();
    for (i = 0; i < transaction_pool_size; i++) {
        print_transaction(pool.transactions[i]);
    }
}

void reset_transaction_pool() {
    memset(&pool, 0, sizeof(pool));
    transaction_pool_size = 0;
}

void reset_ledger() {
    memset(master_ledger->blocks, 0, sizeof(master_ledger->blocks));
    *ledger_size = 0;
    next_block_to_check = 0;
}

void handler(int signal) {
    if (signal == SIGINT) {
        printf("Node received SIGINT\n");
        raise(SIGKILL);
    }
}

void update_info(int index) {
    node_list[index].balance = balance;
    node_list[index].transactions_left = transaction_pool_size;
}