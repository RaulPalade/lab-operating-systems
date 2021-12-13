#include "test.h"
#include "../Unity/unity.h"

ledger (*master_ledger);
transaction_pool pool;

static int ledger_size = 0;
static int block_size = 0;
static int transaction_pool_size = 0;
static int balance = 100;
static int block_id = 0;
static int last_block_confirmed = 0;

configuration conf;

/* SEMAPHORE */
int semaphore;

/* SHARED MEMORY */
int id_shared_memory_legder;

/**
 * MASTER PROCESS
 * 1) Acquire general semaphore to init resources
 * 2) Read configuration
 * 3) Init nodes => assign initial budget through args in execve
 * 4) Init users
 * 5) Release general semaphore 
 * 6) Print node and user budget each second
 * 7) Stop all nodes and users at the end of the simulation
 * 8) Print final report
 */

/**
 * NODE PROCESS
 * 1) Receive transaction from User                     
 * 2) Add transaction received to transaction pool      
 * 3) Add transaction to block                          
 * 4) Add reward transaction to block                   
 * 5) Execute transaction                               
 * 6) Remove transactions from transaction pool         
 */ 

/**
 * USER PROCESS
 * 1) Calculate balance from ledger
 * 2) Extract random user
 * 3) Calculate user revenue (amount)
 * 4) Calculate node revenue (reward)
 * 5) Send transaction
 */ 
int main() {
    key_t key;
    block block;
    transaction sended_transactions[10];

    transaction t1 = new_transaction(111111, 100, 20);
    transaction t2 = new_transaction(222222, 100, 20);
    transaction t3 = new_transaction(222222, 100, 20);
    transaction t4 = new_transaction(111111, 100, 20);
    transaction t5 = new_transaction(111111, 100, 20);
    transaction t6 = new_transaction(111111, 100, 20);

    sended_transactions[0] = t1;

     if ((key = ftok("./test.c", 'a')) < 0) {
        EXIT_ON_ERROR
    }

    if ((id_shared_memory_legder = shmget(key, SO_REGISTRY_SIZE, IPC_CREAT | 0666)) < 0) {
        EXIT_ON_ERROR
    }

    if((void *)(master_ledger = shmat(id_shared_memory_legder, NULL, 0)) < (void *)0) {
        EXIT_ON_ERROR
    }

    block.id = 0;
    add_to_block(&block, t1);
    add_to_block(&block, t2);
    add_to_block(&block, t3);
    add_to_block(&block, t4);
    add_to_block(&block, t5);
    add_to_block(&block, t6);
    
    printf("Add to ledger...\n");
    add_to_ledger(master_ledger, block);

    print_ledger(master_ledger);

    return 0;
}

/* UTIL FUNCTIONS */
void read_configuration(configuration *configuration) {
    FILE *file;
    char s[23];
    char comment;
    char filename[] = "configuration.conf";
    int value;
    int i;
    file = fopen(filename, "r");

    while (fscanf(file, "%s", s) == 1) {
        switch (s[0]) {
            case '#':
                do {
                    comment = fgetc(file);
                } while (comment != '\n');
                break;

            default:
                fscanf(file, "%d\n", &value);
                if (value < 0) {
                    printf("value < 0\n");
                    kill(0, SIGINT);
                }
                if (strncmp(s, "SO_USERS_NUM", 12) == 0) {
                    configuration->SO_USERS_NUM = value;
                } else if (strncmp(s, "SO_NODES_NUM", 12) == 0) {
                    configuration->SO_NODES_NUM = value;
                } else if (strncmp(s, "SO_REWARD", 9) == 0) {
                    configuration->SO_REWARD = value;
                } else if (strncmp(s, "SO_MIN_TRANS_GEN_NSEC", 21) == 0) {
                    configuration->SO_MIN_TRANS_GEN_NSEC = value;
                } else if (strncmp(s, "SO_MAX_TRANS_GEN_NSEC", 21) == 0) {
                    configuration->SO_MAX_TRANS_GEN_NSEC = value;
                } else if (strncmp(s, "SO_RETRY", 8) == 0) {
                    configuration->SO_RETRY = value;
                } else if (strncmp(s, "SO_MIN_TRANS_PROC_NSEC", 22) == 0) {
                    configuration->SO_MIN_TRANS_PROC_NSEC = value;
                } else if (strncmp(s, "SO_MAX_TRANS_PROC_NSEC", 22) == 0) {
                    configuration->SO_MAX_TRANS_PROC_NSEC = value;
                } else if (strncmp(s, "SO_BUDGET_INIT", 14) == 0) {
                    configuration->SO_BUDGET_INIT = value;
                } else if (strncmp(s, "SO_SIM_SEC", 10) == 0) {
                    configuration->SO_SIM_SEC = value;
                } else if (strncmp(s, "SO_FRIENDS_NUM", 14) == 0) {
                    configuration->SO_FRIENDS_NUM = value;
                }
                i++; 
        }
    }
    if (i < 10) {
        printf("Configuration not valid\n");
        kill(0, SIGINT);
    }

    fclose(file);
}

/* Initial semaphore used to init all resources by the master process */
void synchronize_resources(int semaphore) {
    struct sembuf sops;
    sops.sem_num = 0;
    sops.sem_op = 0;
    sops.sem_flg = 0;
    if (semop(semaphore, &sops, 1) < 0) {
        kill(getppid(), SIGUSR1);
        raise(SIGINT);
    }
}

/* For writers and also for readers when readers == 1 */
void acquire_resource(int semaphore, int id_block) {
    struct sembuf sops;
    sops.sem_num = id_block;
    sops.sem_op = -1;
    sops.sem_flg = SEM_UNDO;
    if (semop(semaphore, &sops, 1) < 0) {
        kill(getppid(), SIGUSR1);
        raise(SIGINT);
    }
}

/* For writers and also for readers when readers == 0 */
void release_resource(int semaphore, int id_block) {
    struct sembuf sops;
    sops.sem_num = id_block;
    sops.sem_op = 1;
    sops.sem_flg = SEM_UNDO;
    if (semop(semaphore, &sops, 1) < 0) {
        kill(getppid(), SIGUSR1);
        raise(SIGINT);
    }
}

/* Mutex for readers */
void lock(int semaphore) {
    struct sembuf sops;
    sops.sem_num = 0;
    sops.sem_op = -1;
    sops.sem_flg = SEM_UNDO;
    if (semop(semaphore, &sops, 1) < 0) {
        printf("Error during semop in lock");
        kill(getppid(), SIGUSR1);
        raise(SIGINT);
    }
}

/* Mutex for readers */
void unlock(int semaphore) {
    struct sembuf sops;
    sops.sem_num = 0;
    sops.sem_op = 1;
    sops.sem_flg = SEM_UNDO;
    if (semop(semaphore, &sops, 1) < 0) {
        printf("Error during semop in lock");
        kill(getppid(), SIGUSR1);
        raise(SIGINT);
    }
}

char * get_status(transaction t){
    char *transaction_status;
    if (t.status == UNKNOWN) {
        transaction_status = ANSI_COLOR_MAGENTA "UNKNOWN" ANSI_COLOR_RESET;
    }
    if (t.status == PROCESSING) {
        transaction_status = ANSI_COLOR_YELLOW "PROCESSING" ANSI_COLOR_RESET;
    }
    if (t.status == COMPLETED) {
        transaction_status = ANSI_COLOR_GREEN "COMPLETED" ANSI_COLOR_RESET;
    }
    if (t.status == ABORTED) {
        transaction_status = ANSI_COLOR_RED "ABORTED" ANSI_COLOR_RESET;
    }

    return transaction_status;
}
/* END UTIL FUNCTIONS */


/* MASTER FUNCTIONS */
void execute_node() {

}

void execute_user() {

}
/* END MASTER FUNCTIONS */


/* NODE FUNCTIONS */
int add_to_transaction_pool(transaction t) {
    int added = 0;
    if (transaction_pool_size < SO_TP_SIZE - 1) {
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
        if (pool.transactions[i].timestamp == t.timestamp && pool.transactions[i].sender == t.sender) {
            for (position = i; position < transaction_pool_size; position++) {
                pool.transactions[position] = pool.transactions[position + 1];
            }
            transaction_pool_size--;
            removed = 1;
        }
    }

    return removed;
}

int add_to_block(block *block, transaction t) {
    int added = 0;
    if (block_size < SO_BLOCK_SIZE) {
        (*block).transactions[block_size] = t;
        block_size++;
        added = 1;
    } else {
        printf(ANSI_COLOR_RED "Block size exceeded\n" ANSI_COLOR_RESET);
    }

    printf("Transactions in the block: %d\n", block_size);
    printf("Transactions available for block with id %d:  %d\n", (*block).id, SO_REGISTRY_SIZE - block_size);

    return added;
}

int remove_from_block(block *block, transaction t) {
    int removed = 0;
    int i;
    int position;
    for (i = 0; i < block_size && !removed; i++) {
        if ((*block).transactions[i].timestamp == t.timestamp && (*block).transactions[i].sender == t.sender) {
            for (position = i; position < block_size; position++) {
                (*block).transactions[position] = (*block).transactions[position + 1];
            }
            block_size--;
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
    (*t).status = COMPLETED;

    return executed;
}

int ledger_has_transaction(ledger *ledger, transaction t) {
    int found = 0;
    int i;
    int j;

    for (i = 0; i < ledger_size && !found; i++) {
        for (j = 0; j < block_size; j++) {
            if ((*ledger).blocks[i].transactions[j].timestamp == t.timestamp &&
                (*ledger).blocks[i].transactions[j].sender == t.sender) {
                found = 1;
            }
        }
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

    printf("Blocks in the ledger: %d\n", ledger_size);
    printf("Blocks available: %d\n", SO_REGISTRY_SIZE - ledger_size);

    return added;
}

int remove_from_ledger(ledger *ledger, block block) {
    int removed = 0;
    int i;
    int position;
    for (i = 0; i < block_size && !removed; i++) {
        if ((*ledger).blocks[i].id == block.id) {
            for (position = i; position < block_size; position++) {
                (*ledger).blocks[position] = (*ledger).blocks[position + 1];
            }
            ledger_size--;
            removed = 1;
        }
    }

    return removed;
}

block new_block() {
    block block;
    block.id = block_id;
    block_id++;
}

transaction new_reward_transaction(pid_t receiver, int amount) {
    transaction transaction;
    transaction.timestamp = time(NULL);
    transaction.sender = SENDER_TRANSACTION_REWARD;
    transaction.receiver = 999999;
    transaction.amount = amount;
    transaction.reward = 0;

    return transaction;
}

/* NEED TO CORRECT IF CHECK */
transaction get_random_transaction() {
    transaction transaction;
    int lower = 0;
    int upper = transaction_pool_size;
    int random = 0;
    srand(time(0));
    random = (rand() % (upper - lower + 1)) + lower;

    if (ledger_has_transaction(master_ledger, pool.transactions[random])) {
        get_random_transaction();
    }
    
    transaction = pool.transactions[random];
    return transaction;
}
/* END NODE FUNCTIONS */


/* USER FUNCTIONS */
transaction new_transaction(pid_t receiver, int amount, int reward) {
    transaction transaction;
    int toNode;
    int toUser;
    int random;
    int lower = 2;
    int upper = balance;
    srand(time(NULL));
    random = (rand() % (upper - lower + 1)) + lower;
    toNode = (random * 20) / 100;
    toUser = random - toNode;
    printf("Random=%d,  ToUser=%d, ToNode=%d\n", random, toUser, toNode);
    
    transaction.timestamp = time(NULL);
    transaction.sender = getpid();
    transaction.receiver = receiver;
    transaction.amount = toUser;
    transaction.reward = toNode;
    transaction.status = PROCESSING;
    /* nanosleep(&request, &remaining); */
    
    return transaction;
}

int calculate_balance() {
    int i;
    int j;
    int start_position_ledger = last_block_confirmed * SO_BLOCK_SIZE;    

    for (i = last_block_confirmed; i < ledger_size; i++) {
        for (j = 0; j < block_size; j++) {
            if ((*master_ledger).blocks[i].transactions[j].sender == getpid()) {
                balance = balance - (*master_ledger).blocks[i].transactions[j].amount;
                printf("Sender with amount = %d\n", (*master_ledger).blocks[i].transactions[j].amount);
            }

            if ((*master_ledger).blocks[i].transactions[j].receiver == getpid()) {
                balance = balance + (*master_ledger).blocks[i].transactions[j].amount;
                printf("Receiver with amount = %d\n", (*master_ledger).blocks[i].transactions[j].amount);
            }
        }
        last_block_confirmed++;
    }

    printf("Balance=%d\n", balance);

    return balance;
}

pid_t get_random_user() {
    return 0;
}

pid_t get_random_node() {
    return 0;
}

int handler(int signal) {
    return 0;
}
/* END USER FUNCTIONS */


/* PRINT FUNCTIONS */
void print_configuration(configuration configuration) {
    printf("SO_USERS_NUM = %d\n", configuration.SO_USERS_NUM);
    printf("SO_NODES_NUM = %d\n", configuration.SO_NODES_NUM);
    printf("SO_REWARD = %d\n", configuration.SO_REWARD);
    printf("SO_MIN_TRANS_GEN_NSEC = %d\n", configuration.SO_MIN_TRANS_GEN_NSEC);
    printf("SO_MAX_TRANS_GEN_NSEC = %d\n", configuration.SO_MAX_TRANS_GEN_NSEC);
    printf("SO_RETRY = %d\n", configuration.SO_RETRY);
    printf("SO_MIN_TRANS_PROC_NSEC = %d\n", configuration.SO_MIN_TRANS_PROC_NSEC);
    printf("SO_MAX_TRANS_PROC_NSEC = %d\n", configuration.SO_MAX_TRANS_PROC_NSEC);
    printf("SO_BUDGET_INIT = %d\n", configuration.SO_BUDGET_INIT);
    printf("SO_SIM_SEC = %d\n", configuration.SO_SIM_SEC);
    printf("SO_FRIENDS_NUM = %d\n", configuration.SO_FRIENDS_NUM);
}

void print_transaction(transaction t) {
    char *status = get_status(t);
    printf("%15ld %15d %15d %15d %15d %24s\n", t.timestamp, t.sender, t.receiver, t.amount, t.reward, status);
}

void print_block(block *block) {
    int i;
    printf("BLOCK ID: %d\n", block->id);
    print_table_header();
    for (i = 0; i < block_size; i++) {
        print_transaction(block->transactions[i]);
    }
}

void print_ledger(ledger *ledger) {
    int i;
    for (i = 0; i < ledger_size; i++) {
        print_block(&ledger->blocks[i]);
        printf("-----------------------------------------------------------------------------------------------------\n");
    }
    printf("-----------------------------------------------------------------------------------------------------\n");
}

void print_all_transactions(transaction *transactions) {
    int i;
    print_table_header();
    for (i = 0; i < 3; i++) {
        print_transaction(transactions[i]);
    }
    printf("-----------------------------------------------------------------------------------------------------\n");
}

void print_transaction_pool() {
    int i;
    print_table_header();
    for (i = 0; i < transaction_pool_size; i++) {
        print_transaction(pool.transactions[i]);
    }
}

void print_table_header() {
    printf("-----------------------------------------------------------------------------------------------------\n");
    printf("%15s %15s %15s %15s %15s %15s\n", "TIMESTAMP", "SENDER", "RECEIVER", "AMOUNT", "REWARD", "STATUS");
    printf("-----------------------------------------------------------------------------------------------------\n");
}

void print_live_ledger_info(ledger *ledger) {

}

void print_final_report() {

}
/* END PRINT FUNCTIONS */

/* EXTRA FUNCTIONS */
void unblock(int semaphore) {
    struct sembuf buf;
    buf.sem_num = 0;
    buf.sem_op = -1;
    buf.sem_flg = 0;
    if (semop(semaphore, &buf, 1) < 0) {
        EXIT_ON_ERROR
    } else {
        printf("All resources initialized\n");
    }
}
/* END EXTRA FUNCTIONS */