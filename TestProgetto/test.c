#include "test.h"
#include "../../Unity/unity.h"

ledger (*master_ledger);
transaction_pool pool;

static int ledger_size = 0;
static int transaction_pool_size = 0;
static int balance = 100;
static int block_id = 0;
static int next_block_to_check = 0;

static void test_add_to_transaction_pool();

static void test_remove_from_transaction_pool();

static void test_add_to_block();

static void test_add_to_ledger();

static void test_ledger_has_transaction();

static void test_extract_transaction_block_from_pool();

static void test_calculate_balance();

int main() {

    UNITY_BEGIN();

    RUN_TEST(test_add_to_transaction_pool);
    RUN_TEST(test_remove_from_transaction_pool);
    RUN_TEST(test_add_to_block);
    RUN_TEST(test_add_to_ledger);
    RUN_TEST(test_ledger_has_transaction);
    RUN_TEST(test_extract_transaction_block_from_pool);
    RUN_TEST(test_calculate_balance);

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
    TEST_ASSERT_EQUAL(t_to_add.status, pool.transactions[0].status);
    reset_transaction_pool();
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
    TEST_ASSERT_EQUAL(0, pool.transactions[0].status);
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
        TEST_ASSERT_EQUAL(transactions[i].status, block.transactions[i].status);
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
    free(master_ledger);
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
}

static void test_calculate_balance() {
    int result;
    int tmp_balance;

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

    transaction transactions1[SO_BLOCK_SIZE];
    transaction transactions2[SO_BLOCK_SIZE];
    transaction transactions3[SO_BLOCK_SIZE];

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

    add_to_ledger(master_ledger, block1);
    tmp_balance =
            balance - t1.amount - t1.reward - t2.amount - t2.reward - t3.amount - t3.reward - t4.amount - t4.reward -
            t5.amount - t5.reward;
    result = calculate_balance();
    TEST_ASSERT_EQUAL(tmp_balance, result);
    TEST_ASSERT_EQUAL(1, next_block_to_check);

    add_to_ledger(master_ledger, block2);
    tmp_balance =
            balance - t6.amount - t6.reward - t7.amount - t7.reward - t8.amount - t8.reward - t9.amount - t9.reward -
            t10.amount - t10.reward;
    result = calculate_balance();
    TEST_ASSERT_EQUAL(tmp_balance, result);
    TEST_ASSERT_EQUAL(2, next_block_to_check);

    add_to_ledger(master_ledger, block3);
    tmp_balance = balance - t11.amount - t11.reward - t12.amount - t12.reward - t13.amount - t13.reward - t14.amount -
                  t14.reward - t15.amount - t15.reward;
    result = calculate_balance();
    TEST_ASSERT_EQUAL(tmp_balance, result);
    TEST_ASSERT_EQUAL(3, next_block_to_check);

    reset_ledger(master_ledger);
    free(master_ledger);
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

char *get_status(transaction t) {
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
    (*t).status = COMPLETED;

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
    transaction.timestamp = time(NULL);
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

    srand(time(NULL));

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
    interval.tv_sec = 1;
    interval.tv_nsec = 0;
    srand(time(NULL));
    random = (rand() % (upper - lower + 1)) + lower;
    toNode = (random * 20) / 100;
    toUser = random - toNode;

    transaction.timestamp = time(NULL);
    transaction.sender = getpid();
    transaction.receiver = receiver;
    transaction.amount = toUser;
    transaction.reward = toNode;
    transaction.status = PROCESSING;
    nanosleep(&interval, NULL);

    return transaction;
}

int calculate_balance() {
    int i;
    int j;

    for (i = next_block_to_check; i < ledger_size; i++) {
        for (j = 0; j < SO_BLOCK_SIZE; j++) {
            if ((*master_ledger).blocks[i].transactions[j].sender == getpid()) {
                balance -= ((*master_ledger).blocks[i].transactions[j].amount +
                            (*master_ledger).blocks[i].transactions[j].reward);
            }

            if ((*master_ledger).blocks[i].transactions[j].receiver == getpid()) {
                balance += (*master_ledger).blocks[i].transactions[j].amount;
            }
        }
    }
    next_block_to_check = ledger_size;

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

int equal_transaction(transaction t1, transaction t2) {
    return t1.timestamp == t2.timestamp && t1.sender == t2.sender && t1.receiver == t2.receiver;
}