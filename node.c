#include "util.h"
#include "node.h"

configuration (*config);
ledger (*master_ledger);
transaction_pool pool;

static int ledger_size = 0;
static int transaction_pool_size = 0;
static int balance;
static int next_block_to_check = 0;

int *readers;
int *block_id;

int id_shared_memory_ledger;
int id_shared_memory_configuration;
int id_shared_memory_readers;
int id_shared_memory_block_id;

int id_message_queue_master_node;
int id_message_queue_node_user;

int id_semaphore_init;
int id_semaphore_writers;
int id_semaphore_mutex;

user_node_message user_node_msg;
node_master_message node_master_msg;
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

int main() {
    int i;
    int success;
    transaction *transactions;
    block block;
    int received;
    key_t key;
    struct sigaction sa;

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

    if ((key = ftok("./makefile", 'b')) < 0) {
        raise(SIGQUIT);
    }

    if ((id_shared_memory_readers = shmget(key, 0, 0666)) < 0) {
        raise(SIGQUIT);
    }

    if ((void *) (readers = shmat(id_shared_memory_readers, NULL, 0)) < (void *) 0) {
        raise(SIGQUIT);
    }

    if ((key = ftok("./makefile", 'y')) < 0) {
        raise(SIGQUIT);
    }

    if ((id_shared_memory_block_id = shmget(key, 0, 0666)) < 0) {
        raise(SIGQUIT);
    }

    if ((void *) (block_id = shmat(id_shared_memory_block_id, NULL, 0)) < (void *) 0) {
        raise(SIGQUIT);
    }

    /* MESSAGE QUEUE CONNECTION */
    if ((key = ftok("./makefile", 'c')) < 0) {
        raise(SIGQUIT);
    }
    id_message_queue_master_node = msgget(key, 0666);

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
        gettimeofday(&timer, NULL);
        received = 0;

        printf("Reading transactions\n");
        msgrcv(id_message_queue_node_user, &user_node_msg, sizeof(transaction), 0, 0);

        success = add_to_transaction_pool(user_node_msg.t);
        if (!success) {
            /* INFORM USER TRANSACTION FAILED */
            msgsnd(id_message_queue_node_user, &user_node_msg, sizeof(transaction) - sizeof(long), IPC_NOWAIT);
        }

        if (transaction_pool_size >= SO_BLOCK_SIZE) {
            transactions = extract_transactions_block_from_pool();
            block = new_block(transactions);
            acquire_resource(id_semaphore_writers, block_id);
            success = add_to_ledger(master_ledger, block);
            release_resource(id_semaphore_writers, block_id);

            if (success) {
                for (i = 0; i < SO_BLOCK_SIZE; i++) {
                    remove_from_transaction_pool(transactions[i]);
                }
            }
            free(transactions);
        }
    }

    node_master_msg.args[0] = balance;
    node_master_msg.args[1] = transaction_pool_size;
    msgsnd(id_message_queue_master_node, &node_master_msg, sizeof(node_master_message) - sizeof(long), IPC_NOWAIT);
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

int execute_transaction(transaction *t) {
    int executed = 0;
    struct timespec interval;
    interval.tv_sec = 1;
    interval.tv_nsec = 0;
    printf("Executing transaction...\n");
    nanosleep(&interval, NULL);

    return executed;
}

/* Useless, when a set of transactions are extracted, they are
   not in the ledger because when the block is executed
   automatically those transactions are removed from pool */
int ledger_has_transaction(ledger *ledger, transaction t) {
    int found = 0;
    int i;
    int j;

    for (i = 0; i < ledger_size && !found; i++) {
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
    if (ledger_size < SO_REGISTRY_SIZE) {
        (*ledger).blocks[ledger_size] = block;
        ledger_size++;
        added = 1;
    } else {
        printf(ANSI_COLOR_RED "Ledger size exceeded\n" ANSI_COLOR_RESET);
    }

    return added;
}

block new_block(transaction transactions[]) {
    int i;
    int total_amount;
    int random;
    struct timespec interval;
    int lower = config->SO_MIN_TRANS_PROC_NSEC;
    int upper = config->SO_MAX_TRANS_PROC_NSEC;
    block block;
    srand(time(NULL));
    random = (rand() % (upper - lower)) + lower;

    interval.tv_sec = 0;
    interval.tv_nsec = random;

    block.id = block_id;
    for (i = 0; i < SO_BLOCK_SIZE - 1; i++) {
        block.transactions[i] = transactions[i];
        total_amount += transactions[i].amount;
    }
    block.transactions[SO_BLOCK_SIZE] = new_reward_transaction(total_amount);
    balance += total_amount;
    block_id++;
    nanosleep(&interval, NULL);

    return block;
}

transaction new_reward_transaction(int amount) {
    transaction transaction;
    transaction.timestamp = time(NULL);
    transaction.sender = SENDER_TRANSACTION_REWARD;
    transaction.receiver = getpid();
    transaction.amount = amount;
    transaction.reward = 0;

    return transaction;
}

transaction *extract_transactions_block_from_pool() {
    int i;
    int confirmed = 0;
    int lower = 0;
    int upper = transaction_pool_size;
    int random = 0;
    transaction *transactions = malloc(SO_BLOCK_SIZE * sizeof(transaction));
    int numbers[SO_BLOCK_SIZE - 1];

    srand(time(NULL));

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
    print_table_header();
    for (i = 0; i < transaction_pool_size; i++) {
        print_transaction(pool.transactions[i]);
    }
}

void reset_transaction_pool() {
    memset(&pool, 0, sizeof(pool));
    transaction_pool_size = 0;
}

void reset_ledger(ledger *ledger) {
    memset(master_ledger->blocks, 0, sizeof(master_ledger->blocks));
    ledger_size = 0;
    next_block_to_check = 0;
    block_id = 0;
}

void handler(int signal) {

}