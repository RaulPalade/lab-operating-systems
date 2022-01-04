#include "util.h"

/* Ledger size must be managed in shared memory in order to use print function */

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

void unlock_init_semaphore(int semaphore) {
    struct sembuf buf;
    buf.sem_num = 0;
    buf.sem_op = -1;
    buf.sem_flg = 0;
    if (semop(semaphore, &buf, 1) < 0) {
        EXIT_ON_ERROR
    }
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

int array_contains(transaction *transactions, transaction t) {
    int contains = 0;
    int i;
    for (i = 0; i < SO_BLOCK_SIZE - 1 && !contains; i++) {
        if (equal_transaction(transactions[i], t)) {
            contains = 1;
        }
    }

    return contains;
}

int equal_transaction(transaction t1, transaction t2) {
    return t1.timestamp == t2.timestamp && t1.sender == t2.sender && t1.receiver == t2.receiver;
}

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

void print_all_transactions(transaction *transactions) {
    int i;
    print_table_header();
    for (i = 0; i < 3; i++) {
        print_transaction(transactions[i]);
    }
    printf("-----------------------------------------------------------------------------------------------------\n");
}

void print_table_header() {
    printf("-----------------------------------------------------------------------------------------------------\n");
    printf("%15s %15s %15s %15s %15s\n", "TIMESTAMP", "SENDER", "RECEIVER", "AMOUNT", "REWARD");
    printf("-----------------------------------------------------------------------------------------------------\n");
}

void *new_shared_memory(char proj_id, int id_shm, size_t size) {
    key_t key;
    void *shared_memory;
    if ((key = ftok("./makefile", proj_id)) < 0) {
        EXIT_ON_ERROR
        raise(SIGQUIT);
    }
    if ((id_shm = shmget(key, size, IPC_CREAT | 0666)) < 0) {
        EXIT_ON_ERROR
        raise(SIGQUIT);
    }
    if ((void *) (shared_memory = shmat(id_shm, NULL, 0)) < (void *) 0) {
        EXIT_ON_ERROR
        raise(SIGQUIT);
    }
    return shared_memory;
}

int new_message_queue(char proj_id) {
    key_t key;
    int id;
    if ((key = ftok("./makefile", proj_id)) < 0) {
        EXIT_ON_ERROR
        raise(SIGQUIT);
    }
    if ((id = msgget(key, IPC_CREAT | 0666)) < 0) {
        EXIT_ON_ERROR
        raise(SIGQUIT);
    }
    return id;
}

int new_semaphore(char proj_id) {
    key_t key;
    int id;
    if ((key = ftok("./makefile", proj_id)) < 0) {
        EXIT_ON_ERROR
        raise(SIGQUIT);
    }
    if ((id = semget(key, 1, IPC_CREAT | 0666)) < 0) {
        EXIT_ON_ERROR
        raise(SIGQUIT);
    }
    return id;
}