#include "util.h"

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

// Initial semaphore used to init all resources by the master process
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

// For writers and also for readers when readers == 1
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

// For writers and also for readers when readers == 0
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

// Mutex for readers
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

// Mutex for readers
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

void print_transaction(transaction t) {
    char *status = get_status(t);
    printf("%15ld %15d %15d %15d %15d %24s\n", t.timestamp, t.sender, t.receiver, t.amount, t.reward, status);
}

void print_table_header() {
    printf("-----------------------------------------------------------------------------------------------------\n");
    printf("%15s %15s %15s %15s %15s %15s\n", "TIMESTAMP", "SENDER", "RECEIVER", "AMOUNT", "REWARD", "STATUS");
    printf("-----------------------------------------------------------------------------------------------------\n");
}