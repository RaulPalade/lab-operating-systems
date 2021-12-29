#include "test.h"
#include "../../Unity/unity.h"

configuration config;

ledger (*master_ledger);
transaction_pool pool;
transaction (*processing_transactions);
transaction (*completed_transactions);

static int ledger_size = 0;
static int transaction_pool_size = 0;
static int balance = 100;
static int block_id = 0;
static int next_block_to_check = 0;

static int n_processing_transactions = 0;
static int n_completed_transactions = 0;

static void test_add_to_transaction_pool();

static void test_remove_from_transaction_pool();

static void test_add_to_block();

static void test_add_to_ledger();

static void test_ledger_has_transaction();

static void test_extract_transaction_block_from_pool();

static void test_calculate_balance();

int main() {
    read_configuration(&config);
    UNITY_BEGIN();

    pool.transactions = malloc(config.SO_TP_SIZE * sizeof(transaction));
    RUN_TEST(test_add_to_transaction_pool);
    reset_transaction_pool();

    pool.transactions = malloc(config.SO_TP_SIZE * sizeof(transaction));
    RUN_TEST(test_remove_from_transaction_pool);
    reset_transaction_pool();

    /* RUN_TEST(test_add_to_block);
    RUN_TEST(test_add_to_ledger);
    RUN_TEST(test_ledger_has_transaction);
    RUN_TEST(test_extract_transaction_block_from_pool);
    RUN_TEST(test_calculate_balance); */

    UNITY_END();

    return 0;
}

static void test_add_to_transaction_pool() {
    transaction t_to_add = new_transaction(111111, 100, 20);
    int result = add_to_transaction_pool(t_to_add);

    TEST_ASSERT_EQUAL(1, result);
    TEST_ASSERT_EQUAL(1, transaction_pool_size);
    TEST_ASSERT_EQUAL(t_to_add.timestamp, pool.transactions[0].timestamp);
    TEST_ASSERT_EQUAL(t_to_add.sender, pool.transactions[0].sender);
    TEST_ASSERT_EQUAL(t_to_add.receiver, pool.transactions[0].receiver);
    TEST_ASSERT_EQUAL(t_to_add.amount, pool.transactions[0].amount);
    TEST_ASSERT_EQUAL(t_to_add.reward, pool.transactions[0].reward);

    /* reset_transaction_pool(); */
}

static void test_remove_from_transaction_pool() {
    transaction t_to_remove = new_transaction(111111, 100, 20);
    int result;

    add_to_transaction_pool(t_to_remove);
    result = remove_from_transaction_pool(t_to_remove);

    TEST_ASSERT_EQUAL(1, result);
    TEST_ASSERT_EQUAL(0, pool.transactions[0].timestamp);
    TEST_ASSERT_EQUAL(0, pool.transactions[0].sender);
    TEST_ASSERT_EQUAL(0, pool.transactions[0].receiver);
    TEST_ASSERT_EQUAL(0, pool.transactions[0].amount);
    TEST_ASSERT_EQUAL(0, pool.transactions[0].reward);

    reset_transaction_pool();
}

static void test_add_to_block() {
    int i;
    block block;

    transaction t1 = new_transaction(111111, 100, 20);
    transaction t2 = new_transaction(111111, 100, 20);
    transaction t3 = new_transaction(111111, 100, 20);
    transaction t4 = new_transaction(111111, 100, 20);
    transaction t5 = new_transaction(111111, 100, 20);

    transaction transactions[SO_BLOCK_SIZE];
    transactions[0] = t1;
    transactions[1] = t2;
    transactions[2] = t3;
    transactions[3] = t4;
    transactions[4] = t5;

    block = new_block(transactions);

    TEST_ASSERT_EQUAL(1, block_id);
    for (i = 0; i < SO_BLOCK_SIZE; i++) {
        TEST_ASSERT_EQUAL(transactions[i].timestamp, block.transactions[i].timestamp);
        TEST_ASSERT_EQUAL(transactions[i].sender, block.transactions[i].sender);
        TEST_ASSERT_EQUAL(transactions[i].receiver, block.transactions[i].receiver);
        TEST_ASSERT_EQUAL(transactions[i].amount, block.transactions[i].amount);
        TEST_ASSERT_EQUAL(transactions[i].reward, block.transactions[i].reward);
    }
}

static void test_add_to_ledger() {
    int result;
    block block;

    transaction t1 = new_transaction(111111, 100, 20);
    transaction t2 = new_transaction(111111, 100, 20);
    transaction t3 = new_transaction(111111, 100, 20);
    transaction t4 = new_transaction(111111, 100, 20);
    transaction t5 = new_transaction(111111, 100, 20);

    transaction transactions[SO_BLOCK_SIZE];
    transactions[0] = t1;
    transactions[1] = t2;
    transactions[2] = t3;
    transactions[3] = t4;
    transactions[4] = t5;

    block = new_block(transactions);

    master_ledger = malloc(SO_REGISTRY_SIZE * sizeof(ledger));

    result = add_to_ledger(master_ledger, block);
    TEST_ASSERT_EQUAL(1, ledger_size);

    reset_ledger(master_ledger);
}

static void test_ledger_has_transaction() {
    int result;
    block block;

    transaction t1 = new_transaction(111111, 100, 20);
    transaction t2 = new_transaction(111111, 100, 20);
    transaction t3 = new_transaction(111111, 100, 20);
    transaction t4 = new_transaction(111111, 100, 20);
    transaction t5 = new_transaction(111111, 100, 20);

    transaction transactions[SO_BLOCK_SIZE];
    transactions[0] = t1;
    transactions[1] = t2;
    transactions[2] = t3;
    transactions[3] = t4;
    transactions[4] = t5;

    block = new_block(transactions);

    master_ledger = malloc(SO_REGISTRY_SIZE * sizeof(ledger));

    add_to_ledger(master_ledger, block);
    result = ledger_has_transaction(master_ledger, t1);
    TEST_ASSERT_EQUAL(1, result);
    result = ledger_has_transaction(master_ledger, t2);
    TEST_ASSERT_EQUAL(1, result);
    result = ledger_has_transaction(master_ledger, t3);
    TEST_ASSERT_EQUAL(1, result);
    result = ledger_has_transaction(master_ledger, t4);
    TEST_ASSERT_EQUAL(1, result);
    result = ledger_has_transaction(master_ledger, t5);
    TEST_ASSERT_EQUAL(1, result);

    reset_ledger(master_ledger);
}

static void test_extract_transaction_block_from_pool() {
    int i;
    int j;
    int found = 0;
    int equals = 0;
    int totalChecks = 0;

    transaction *transactions;
    transaction t1 = new_transaction(111111, 100, 20);
    transaction t2 = new_transaction(111111, 100, 20);
    transaction t3 = new_transaction(111111, 100, 20);
    transaction t4 = new_transaction(111111, 100, 20);
    transaction t5 = new_transaction(111111, 100, 20);
    transaction t6 = new_transaction(111111, 100, 20);
    transaction t7 = new_transaction(111111, 100, 20);
    transaction t8 = new_transaction(111111, 100, 20);
    transaction t9 = new_transaction(111111, 100, 20);
    transaction t10 = new_transaction(111111, 100, 20);

    add_to_transaction_pool(t1);
    add_to_transaction_pool(t2);
    add_to_transaction_pool(t3);
    add_to_transaction_pool(t4);
    add_to_transaction_pool(t5);
    add_to_transaction_pool(t6);
    add_to_transaction_pool(t7);
    add_to_transaction_pool(t8);
    add_to_transaction_pool(t9);
    add_to_transaction_pool(t10);

    transactions = extract_transaction_block_from_pool();

    for (i = 0; i < SO_BLOCK_SIZE; i++) {
        for (j = 0; j < transaction_pool_size && !found; j++) {
            if (equal_transaction(transactions[i], pool.transactions[j])) {
                found = 1;
                equals++;
            }
            totalChecks++;
        }
        found = 0;
    }

    TEST_ASSERT_EQUAL(5, equals);
    free(transactions);
}

static void test_calculate_balance() {
    int result;
    int tmp_balance;
    int processing_balance;

    block block1;
    block block2;
    block block3;

    transaction t1 = new_transaction(111111, 100, 20);
    transaction t2 = new_transaction(111111, 100, 20);
    transaction t3 = new_transaction(111111, 100, 20);
    transaction t4 = new_transaction(111111, 100, 20);
    transaction t5 = new_transaction(111111, 100, 20);

    transaction t6 = new_transaction(111111, 100, 20);
    transaction t7 = new_transaction(111111, 100, 20);
    transaction t8 = new_transaction(111111, 100, 20);
    transaction t9 = new_transaction(111111, 100, 20);
    transaction t10 = new_transaction(111111, 100, 20);

    transaction t11 = new_transaction(111111, 100, 20);
    transaction t12 = new_transaction(111111, 100, 20);
    transaction t13 = new_transaction(111111, 100, 20);
    transaction t14 = new_transaction(111111, 100, 20);
    transaction t15 = new_transaction(111111, 100, 20);

    transaction t16 = new_transaction(111111, 100, 20);
    transaction t17 = new_transaction(111111, 100, 20);
    transaction t18 = new_transaction(111111, 100, 20);
    transaction t19 = new_transaction(111111, 100, 20);
    transaction t20 = new_transaction(111111, 100, 20);

    transaction transactions1[SO_BLOCK_SIZE];
    transaction transactions2[SO_BLOCK_SIZE];
    transaction transactions3[SO_BLOCK_SIZE];

    master_ledger = malloc(SO_REGISTRY_SIZE * sizeof(ledger));

    processing_transactions = malloc(0 * sizeof(transaction));
    completed_transactions = malloc(0 * sizeof(transaction));

    add_to_processing_list(t1);
    add_to_processing_list(t2);
    add_to_processing_list(t3);
    add_to_processing_list(t4);

    add_to_processing_list(t16);
    add_to_processing_list(t17);
    add_to_processing_list(t18);
    add_to_processing_list(t19);
    add_to_processing_list(t20);

    transactions1[0] = t1;
    transactions1[1] = t2;
    transactions1[2] = t3;
    transactions1[3] = t4;
    transactions1[4] = t5;

    transactions2[0] = t6;
    transactions2[1] = t7;
    transactions2[2] = t8;
    transactions2[3] = t9;
    transactions2[4] = t10;

    transactions3[0] = t11;
    transactions3[1] = t12;
    transactions3[2] = t13;
    transactions3[3] = t14;
    transactions3[4] = t15;

    block1 = new_block(transactions1);
    block2 = new_block(transactions2);
    block3 = new_block(transactions3);

    processing_balance =
            t16.amount + t16.reward + t17.amount + t17.reward + t18.amount + t18.reward + t19.amount + t19.reward +
            t20.amount + t20.reward;

    add_to_ledger(master_ledger, block1);
    tmp_balance =
            balance - t1.amount - t1.reward - t2.amount - t2.reward - t3.amount - t3.reward - t4.amount - t4.reward -
            t5.amount - t5.reward;
    tmp_balance -= processing_balance;
    result = calculate_balance();
    TEST_ASSERT_EQUAL(tmp_balance, result);
    TEST_ASSERT_EQUAL(1, next_block_to_check);

    add_to_ledger(master_ledger, block2);
    tmp_balance =
            balance - t6.amount - t6.reward - t7.amount - t7.reward - t8.amount - t8.reward - t9.amount - t9.reward -
            t10.amount - t10.reward;
    tmp_balance -= processing_balance;
    result = calculate_balance();
    TEST_ASSERT_EQUAL(tmp_balance, result);
    TEST_ASSERT_EQUAL(2, next_block_to_check);

    add_to_ledger(master_ledger, block3);
    tmp_balance = balance - t11.amount - t11.reward - t12.amount - t12.reward - t13.amount - t13.reward - t14.amount -
                  t14.reward - t15.amount - t15.reward;
    tmp_balance -= processing_balance;
    result = calculate_balance();
    TEST_ASSERT_EQUAL(tmp_balance, result);
    TEST_ASSERT_EQUAL(3, next_block_to_check);

    free(processing_transactions);
    free(completed_transactions);
    reset_ledger(master_ledger);
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
                } else if (strncmp(s, "SO_TP_SIZE", 10) == 0) {
                    configuration->SO_TP_SIZE = value;
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
    if (transaction_pool_size < config.SO_TP_SIZE - 1) {
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
    block block;
    block.id = block_id;
    for (i = 0; i < SO_BLOCK_SIZE; i++) {
        block.transactions[i] = transactions[i];
    }
    block_id++;

    return block;
}

transaction new_reward_transaction(pid_t receiver, int amount) {
    transaction transaction;
    struct timespec tp;
    clockid_t clock_id;
    clock_gettime(clock_id, &tp);

    transaction.timestamp = tp.tv_sec;
    transaction.sender = SENDER_TRANSACTION_REWARD;
    transaction.receiver = 999999;
    transaction.amount = amount;
    transaction.reward = 0;

    return transaction;
}

transaction *extract_transaction_block_from_pool() {
    int i;
    int confirmed = 0;
    int lower = 0;
    int upper = transaction_pool_size;
    int random = 0;
    transaction *transactions = malloc(SO_BLOCK_SIZE * sizeof(transaction));
    int numbers[SO_BLOCK_SIZE];

    struct timespec tp;
    clockid_t clock_id;

    clock_gettime(clock_id, &tp);
    srand(tp.tv_sec);

    while (confirmed < SO_BLOCK_SIZE) {
        random = (rand() % (upper - lower)) + lower;
        if (!array_contains(numbers, random)) {
            numbers[confirmed] = random;
            transactions[confirmed] = pool.transactions[random];
            confirmed++;
        }
    }

    return transactions;
}
/* END NODE FUNCTIONS */


/* USER FUNCTIONS */
transaction new_transaction(pid_t receiver, int amount, int reward) {
    transaction transaction;
    struct timespec interval;
    int toNode;
    int toUser;
    int random;
    int lower = 2;
    int upper = balance;
    struct timespec tp;
    clockid_t clock_id;
    interval.tv_sec = 1;
    interval.tv_nsec = 0;
    
    clock_gettime(clock_id, &tp);
    srand(tp.tv_sec);
    random = (rand() % (upper - lower + 1)) + lower;
    toNode = (random * 20) / 100;
    toUser = random - toNode;

    transaction.timestamp = tp.tv_sec;
    transaction.sender = getpid();
    transaction.receiver = receiver;
    transaction.amount = toUser;
    transaction.reward = toNode;
    nanosleep(&interval, NULL);

    return transaction;
}

int calculate_balance() {
    int i;
    int j;
    int y;

    for (i = next_block_to_check; i < ledger_size; i++) {
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
    next_block_to_check = ledger_size;

    for (y = 0; y < n_processing_transactions; y++) {
        balance -= processing_transactions[y].amount + processing_transactions[y].reward;
    }

    return balance;
}

pid_t get_random_user() {
    return 0;
}

pid_t get_random_node() {
    return 0;
}

void handler(int signal) {

}

int equal_transaction(transaction t1, transaction t2) {
    return t1.timestamp == t2.timestamp && t1.sender == t2.sender && t1.receiver == t2.receiver;
}

int remove_from_processing_list(int position) {
    int removed = 0;
    int i;
    for (i = position; i < n_processing_transactions; i++) {
        processing_transactions[i] = processing_transactions[i + 1];
    }
    n_processing_transactions--;
    processing_transactions = realloc(processing_transactions, (n_processing_transactions) * sizeof(transaction));
    removed = 1;

    return removed;
}

int add_to_processing_list(transaction t) {
    processing_transactions = realloc(processing_transactions, (n_processing_transactions + 1) * sizeof(transaction));
    processing_transactions[n_processing_transactions] = t;
    n_processing_transactions++;
}

int add_to_completed_list(transaction t) {
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
/* END USER FUNCTIONS */


/* PRINT FUNCTIONS */
void print_configuration(configuration configuration) {
    printf("SO_USERS_NUM = %d\n", configuration.SO_USERS_NUM);
    printf("SO_NODES_NUM = %d\n", configuration.SO_NODES_NUM);
    printf("SO_REWARD = %d\n", configuration.SO_REWARD);
    printf("SO_MIN_TRANS_GEN_NSEC = %d\n", configuration.SO_MIN_TRANS_GEN_NSEC);
    printf("SO_MAX_TRANS_GEN_NSEC = %d\n", configuration.SO_MAX_TRANS_GEN_NSEC);
    printf("SO_RETRY = %d\n", configuration.SO_RETRY);
    printf("SO_TP_SIZE = %d\n", configuration.SO_TP_SIZE);
    printf("SO_MIN_TRANS_PROC_NSEC = %d\n", configuration.SO_MIN_TRANS_PROC_NSEC);
    printf("SO_MAX_TRANS_PROC_NSEC = %d\n", configuration.SO_MAX_TRANS_PROC_NSEC);
    printf("SO_BUDGET_INIT = %d\n", configuration.SO_BUDGET_INIT);
    printf("SO_SIM_SEC = %d\n", configuration.SO_SIM_SEC);
    printf("SO_FRIENDS_NUM = %d\n", configuration.SO_FRIENDS_NUM);
}

void print_transaction(transaction t) {
    printf("%15ld %15d %15d %15d %15d\n", t.timestamp, t.sender, t.receiver, t.amount, t.reward);
}

void print_block(block block) {
    int i;
    printf("BLOCK ID: %d\n", block.id);
    print_table_header();
    for (i = 0; i < SO_BLOCK_SIZE; i++) {
        print_transaction(block.transactions[i]);
    }
}

void print_ledger(ledger *ledger) {
    int i;
    for (i = 0; i < ledger_size; i++) {
        print_block(ledger->blocks[i]);
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
    printf("%15s %15s %15s %15s %15s\n", "TIMESTAMP", "SENDER", "RECEIVER", "AMOUNT", "REWARD");
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

void reset_transaction_pool() {
    memset(&pool.transactions, 0, sizeof(pool));
    transaction_pool_size = 0;
}

void reset_ledger(ledger *ledger) {
    memset(master_ledger->blocks, 0, sizeof(master_ledger->blocks));
    ledger_size = 0;
    next_block_to_check = 0;
    block_id = 0;
    free(master_ledger);
}

int array_contains(int array[], int element) {
    int contains = 0;
    int i;
    for (i = 0; i < SO_BLOCK_SIZE && !contains; i++) {
        if (array[i] == element) {
            contains = 1;
        }
    }

    return contains;
}