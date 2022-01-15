#include "../include/master.h"
#include "../include/siglib.h"

#define N_BALANCE_PRINT 10

configuration config;
ledger (*master_ledger);
pid_t *user_list;
int *block_id;
int *readers_block_id;

/* SHARED MEMORY */
int id_shm_ledger;
int id_shm_user_list;
int id_shm_block_id;
int id_shm_readers_block_id;

/* MESSAGE QUEUE */
int id_msg_node_user;
int id_msg_user_node;

/* SEMAPHORE */
int id_sem_init;
int id_sem_writers_block_id;
int id_sem_readers_block_id;

/* SIMULATION RUNNING PARAMETERS */
volatile int executing = 1;
int ledger_full = 0;
int active_nodes = 0;
int active_users = 0;

int initial_total_funds;
int final_total_funds;

pid_t *node_list;

/**
 * MASTER PROCESS
 * 1) Acquire general semaphore to init resources
 * 2) Read configuration
 * 3) Init nodes => assign initial budget through args in execv
 * 4) Init users
 * 5) Release general semaphore 
 * 6) Print node and user budget each second
 * 7) Stop all nodes and users at the end of the simulation
 * 8) Print final report
 */
int main() {
    int sem_value;
    int i;
    int node_balance;
    int user_balance;
    user_data top_list[N_USER_TO_DISPLAY];
    user_data min_list[N_USER_TO_DISPLAY];

    char *args_node[12] = {NODE};
    char *args_user[17] = {USER};
    char index_node[3 * sizeof(int) + 1];
    char index_user[3 * sizeof(int) + 1];

    char so_tp_size[3 * sizeof(int) + 1];
    char so_min_trans_proc_nsec[3 * sizeof(int) + 1];
    char so_max_trans_proc_nsec[3 * sizeof(int) + 1];

    char so_budget_init[3 * sizeof(int) + 1];
    char so_retry[3 * sizeof(int) + 1];
    char so_min_trans_gen_nsec[3 * sizeof(int) + 1];
    char so_max_trans_gen_nsec[3 * sizeof(int) + 1];
    char so_nodes_num[3 * sizeof(int) + 1];
    char so_users_num[3 * sizeof(int) + 1];

    char id_shm_ledger_str[3 * sizeof(int) + 1];
    char id_shm_block_id_str[3 * sizeof(int) + 1];
    char id_shm_readers_block_id_str[3 * sizeof(int) + 1];
    char id_shm_user_list_str[3 * sizeof(int) + 1];
    char id_msg_node_user_str[3 * sizeof(int) + 1];
    char id_msg_user_node_str[3 * sizeof(int) + 1];
    char id_sem_init_str[3 * sizeof(int) + 1];
    char id_sem_writers_block_id_str[3 * sizeof(int) + 1];
    char id_sem_readers_block_id_str[3 * sizeof(int) + 1];
    key_t key;
    int remaining_seconds;
    pid_t node_pid;
    pid_t user_pid;
    struct sigaction sa;
    struct timespec request;
    struct timespec remaining;
    union semun sem_ds;
    sigset_t my_mask;
    request.tv_sec = 1;
    request.tv_nsec = 0;
    remaining.tv_sec = 1;
    remaining.tv_nsec = 0;

    mask(SIGINT, my_mask);
    mask(SIGTERM, my_mask);
    mask(SIGQUIT, my_mask);
    bzero(&sa, sizeof(sa));
    sa.sa_handler = handler;
    sigaction(SIGALRM, &sa, 0);
    sigaction(SIGINT, &sa, 0);
    sigaction(SIGTERM, &sa, 0);
    sigaction(SIGQUIT, &sa, 0);
    sigaction(SIGCHLD, &sa, 0);
    sigaction(SIGUSR1, &sa, 0);
    sigaction(SIGUSR2, &sa, 0);

    read_configuration(&config);

    node_list = malloc(config.SO_NODES_NUM * sizeof(pid_t));

    /* SHARED MEMORY CREATION */
    key = ftok("../makefile", PROJ_ID_SHM_LEDGER);
    EXIT_ON_ERROR
    id_shm_ledger = shmget(key, sizeof(block) * SO_REGISTRY_SIZE, IPC_CREAT | 0666);
    EXIT_ON_ERROR
    master_ledger = shmat(id_shm_ledger, NULL, 0);
    EXIT_ON_ERROR

    key = ftok("../makefile", PROJ_ID_SHM_USER_LIST);
    EXIT_ON_ERROR
    id_shm_user_list = shmget(key, sizeof(pid_t) * config.SO_USERS_NUM, IPC_CREAT | 0666);
    EXIT_ON_ERROR
    user_list = shmat(id_shm_user_list, NULL, 0);

    key = ftok("../makefile", PROJ_ID_SHM_BLOCK_ID);
    EXIT_ON_ERROR
    id_shm_block_id = shmget(key, sizeof(block_id), IPC_CREAT | 0666);
    EXIT_ON_ERROR
    block_id = shmat(id_shm_block_id, NULL, 0);
    EXIT_ON_ERROR
    *block_id = 0;

    key = ftok("../makefile", PROJ_ID_SHM_READERS_BLOCK_ID);
    EXIT_ON_ERROR
    id_shm_readers_block_id = shmget(key, sizeof(readers_block_id), IPC_CREAT | 0666);
    EXIT_ON_ERROR
    readers_block_id = shmat(id_shm_readers_block_id, NULL, 0);
    EXIT_ON_ERROR
    *readers_block_id = 0;

    /* MESSAGE QUEUE CREATION */
    key = ftok("../makefile", PROJ_ID_MSG_NODE_USER);
    EXIT_ON_ERROR
    id_msg_node_user = msgget(key, IPC_CREAT | 0666);
    EXIT_ON_ERROR

    key = ftok("../makefile", PROJ_ID_MSG_USER_NODE);
    EXIT_ON_ERROR
    id_msg_user_node = msgget(key, IPC_CREAT | 0666);
    EXIT_ON_ERROR

    /* SEMAPHORE CREATION */
    key = ftok("../makefile", PROJ_ID_SEM_INIT);
    EXIT_ON_ERROR
    id_sem_init = semget(key, 1, IPC_CREAT | 0666);
    EXIT_ON_ERROR
    set_semaphore_val(id_sem_init, 0, 1);

    key = ftok("../makefile", PROJ_ID_SEM_WRITERS);
    EXIT_ON_ERROR
    id_sem_writers_block_id = semget(key, 1, IPC_CREAT | 0666);
    EXIT_ON_ERROR
    set_semaphore_val(id_sem_writers_block_id, 0, 1);

    key = ftok("../makefile", PROJ_ID_SEM_READERS_BLOCK_ID);
    EXIT_ON_ERROR
    id_sem_readers_block_id = semget(key, 1, IPC_CREAT | 0666);
    EXIT_ON_ERROR
    set_semaphore_val(id_sem_readers_block_id, 0, 1);

    unmask(SIGINT, my_mask);
    unmask(SIGTERM, my_mask);
    unmask(SIGINT, my_mask);
    unmask(SIGQUIT, my_mask);

    /* Preparing cmd line arguments for execv */
    sprintf(so_tp_size, "%d", config.SO_TP_SIZE);
    sprintf(so_min_trans_proc_nsec, "%d", config.SO_MIN_TRANS_PROC_NSEC);
    sprintf(so_max_trans_proc_nsec, "%d", config.SO_MAX_TRANS_PROC_NSEC);

    sprintf(so_budget_init, "%d", config.SO_BUDGET_INIT);
    sprintf(so_retry, "%d", config.SO_RETRY);
    sprintf(so_min_trans_gen_nsec, "%d", config.SO_MIN_TRANS_GEN_NSEC);
    sprintf(so_max_trans_gen_nsec, "%d", config.SO_MAX_TRANS_GEN_NSEC);
    sprintf(so_nodes_num, "%d", config.SO_NODES_NUM);
    sprintf(so_users_num, "%d", config.SO_USERS_NUM);

    sprintf(id_shm_ledger_str, "%d", id_shm_ledger);
    sprintf(id_shm_user_list_str, "%d", id_shm_user_list);
    sprintf(id_shm_block_id_str, "%d", id_shm_block_id);
    sprintf(id_shm_readers_block_id_str, "%d", id_shm_readers_block_id);

    sprintf(id_msg_node_user_str, "%d", id_msg_node_user);
    sprintf(id_msg_user_node_str, "%d", id_msg_user_node);

    sprintf(id_sem_init_str, "%d", id_sem_init);
    sprintf(id_sem_writers_block_id_str, "%d", id_sem_writers_block_id);
    sprintf(id_sem_readers_block_id_str, "%d", id_sem_readers_block_id);

    args_node[2] = so_tp_size;
    args_node[3] = so_min_trans_proc_nsec;
    args_node[4] = so_max_trans_proc_nsec;
    args_node[5] = id_shm_ledger_str;
    args_node[6] = id_shm_block_id_str;
    args_node[7] = id_msg_node_user_str;
    args_node[8] = id_msg_user_node_str;
    args_node[9] = id_sem_init_str;
    args_node[10] = id_sem_writers_block_id_str;
    args_node[11] = NULL;

    args_user[2] = so_budget_init;
    args_user[3] = so_retry;
    args_user[4] = so_min_trans_gen_nsec;
    args_user[5] = so_max_trans_gen_nsec;
    args_user[6] = so_users_num;
    args_user[7] = id_shm_ledger_str;
    args_user[8] = id_shm_user_list_str;
    args_user[9] = id_shm_block_id_str;
    args_user[10] = id_shm_readers_block_id_str;
    args_user[11] = id_msg_node_user_str;
    args_user[12] = id_msg_user_node_str;
    args_user[13] = id_sem_init_str;
    args_user[14] = id_sem_writers_block_id_str;
    args_user[15] = id_sem_readers_block_id_str;
    args_user[16] = NULL;

    for (i = 0; i < config.SO_NODES_NUM; i++) {
        switch (node_pid = fork()) {
            case -1:
                EXIT_ON_ERROR

            case 0:
                sprintf(index_node, "%d", i);
                args_node[1] = index_node;
                execv("node", args_node);

            default:
                node_list[i] = node_pid;
                active_nodes++;
        }
    }

    for (i = 0; i < config.SO_USERS_NUM; i++) {
        switch (user_pid = fork()) {
            case -1:
                EXIT_ON_ERROR

            case 0:
                sprintf(index_user, "%d", i);
                args_user[1] = index_user;
                execv("user", args_user);

            default:
                user_list[i] = user_pid;
                initial_total_funds += 1000;
                active_users++;
        }
    }

    init_array(top_list, INT_MIN);
    init_array(min_list, INT_MAX);
    remaining_seconds = config.SO_SIM_SEC;
    alarm(config.SO_SIM_SEC);
    unlock_init_semaphore(id_sem_init);
    while (executing && !ledger_full && active_users > 0) {
        printf("TIMER = %d\n", remaining_seconds--);
        for (i = 0; i < config.SO_NODES_NUM; i++) {
            node_balance = calculate_node_balance(node_list[i]);
            printf("Node %d balance = %d\n", node_list[i], node_balance);
        }

        for (i = 0; i < config.SO_USERS_NUM; i++) {
            user_balance = calculate_user_balance(user_list[i]);
            add_max(top_list, user_list[i], user_balance);
            add_min(min_list, user_list[i], user_balance);
            printf("\nTOP USERS\n");
            print_user_data(top_list);
            printf("\nMIN USERS\n");
            print_user_data(min_list);
        }
        nanosleep(&request, &remaining);
    }

    kill(0, SIGTERM);

    print_ledger();

    for (i = 0; i < config.SO_NODES_NUM; i++) {
        node_balance = calculate_node_balance(node_list[i]);
        printf("Node %d balance = %d\n", node_list[i], node_balance);
    }
    for (i = 0; i < config.SO_USERS_NUM; i++) {
        user_balance = calculate_user_balance(user_list[i]);
        printf("User %d balance = %d\n", user_list[i], user_balance);
    }

    print_final_report();

    cleanIPC();

    return 0;
}

int calculate_user_balance(pid_t user) {
    int user_balance = config.SO_BUDGET_INIT;
    int i;
    int j;
    int tmp_block_id;
    lock(id_sem_writers_block_id);
    tmp_block_id = *block_id;
    unlock(id_sem_writers_block_id);
    for (i = 0; i < tmp_block_id; i++) {
        for (j = 0; j < SO_BLOCK_SIZE - 1; j++) {
            if (master_ledger->blocks[i].transactions[j].sender == user) {
                user_balance -= master_ledger->blocks[i].transactions[j].amount;
                user_balance -= master_ledger->blocks[i].transactions[j].reward;
            } else if (master_ledger->blocks[i].transactions[j].receiver == user) {
                user_balance += master_ledger->blocks[i].transactions[j].amount;
            }
        }
    }
    return user_balance;
}

int calculate_node_balance(pid_t node) {
    int node_balance = 0;
    int i;
    int j = SO_BLOCK_SIZE - 1;
    int tmp_block_id;

    lock(id_sem_writers_block_id);
    tmp_block_id = *block_id;
    unlock(id_sem_writers_block_id);
    for (i = 0; i < tmp_block_id; i++) {
        if (master_ledger->blocks[i].transactions[j].receiver == node) {
            node_balance += master_ledger->blocks[i].transactions[j].amount;
        }
    }

    return node_balance;
}

void add_max(user_data *array, pid_t pid, int balance) {
    int i;
    int added = 0;
    int present = 0;
    for (i = 0; i < N_USER_TO_DISPLAY && !added; i++) {
        if (pid == array[i].pid) {
            if (balance > array[i].balance) {
                array[i].balance = balance;
                qsort(&array[i].balance, N_USER_TO_DISPLAY, sizeof(int), compare);
                added = 1;
            }
            present = 1;
        }
        if (present == 0 && balance > array[i].balance) {
            memmove(array + i + 1, array + i, (N_USER_TO_DISPLAY - i - 1) * sizeof(*array));
            array[i].pid = pid;
            array[i].balance = balance;
            added = 1;
        }
    }
}

void add_min(user_data *array, pid_t pid, int balance) {
    int i;
    int added = 0;
    int present = 0;
    for (i = 0; i < N_USER_TO_DISPLAY && !added; i++) {
        if (pid == array[i].pid) {
            if (balance < array[i].balance) {
                array[i].balance = balance;
                qsort(&array[i].balance, N_USER_TO_DISPLAY, sizeof(int), compare);
                added = 1;
            }
            present = 1;
        }
        if (present == 0 && balance < array[i].balance) {
            memmove(array + i + 1, array + i, (N_USER_TO_DISPLAY - i - 1) * sizeof(*array));
            array[i].pid = pid;
            array[i].balance = balance;
            added = 1;
        }
    }
}

void init_array(user_data *array, int value) {
    int i;
    for (i = 0; i < N_USER_TO_DISPLAY; i++) {
        array[i].pid = 0;
        array[i].balance = value;
    }
}

void print_user_data(user_data *array) {
    int i;
    printf("%5s%10s\n", "PID", "BALANCE");
    printf("---------------------------\n");
    for (i = 0; i < N_USER_TO_DISPLAY; i++) {
        printf("%5d%10d\n", array[i].pid, array[i].balance);
    }
}

int compare(const void *a, const void *b) {
    return (*(int *) a - *(int *) b);
}

void print_ledger() {
    int i;
    int tmp_block_id;
    printf("Printing ledger\n");
    lock(id_sem_writers_block_id);
    tmp_block_id = *block_id;
    unlock(id_sem_writers_block_id);
    for (i = 0; i < tmp_block_id; i++) {
        print_block(master_ledger->blocks[i]);
        printf("-----------------------------------------------------------------------------------------------------\n");
    }
    printf("-----------------------------------------------------------------------------------------------------\n");
}

void print_live_info() {
    printf("-----------------------------------------------------------------------------------------------------\n");
    printf("%10s%10s\n", "ACTIVE NODES", "ACTIVE USERS");
    printf("%10d%10d\n", active_nodes, active_users);
}

void print_final_report() {
    printf("\n---------------------END---------------------\n");
    printf("Simulation ended with flags\n");
    if (active_users == 0) {
        printf("Simulation ended because all users are dead\n");
    } else if (executing == 0) {
        printf("Simulation ended because time expired\n");
    } else if (ledger_full == 0) {
        printf("Simulation ended because the ledger is full\n");
    }
    printf("User processes dead = %d\n", config.SO_USERS_NUM - active_users);
    printf("Number of blocks in the ledger = %d\n", *block_id);
    printf("---------------------END---------------------\n");
}

void read_configuration(configuration *config) {
    FILE *file;
    char s[23];
    char comment;
    char filename[] = "../configurations/configuration2.conf";
    int value;
    int i;

    if (access(filename, F_OK) == 0) {
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
                        config->SO_USERS_NUM = value;
                    } else if (strncmp(s, "SO_NODES_NUM", 12) == 0) {
                        config->SO_NODES_NUM = value;
                    } else if (strncmp(s, "SO_REWARD", 9) == 0) {
                        config->SO_REWARD = value;
                    } else if (strncmp(s, "SO_MIN_TRANS_GEN_NSEC", 21) == 0) {
                        config->SO_MIN_TRANS_GEN_NSEC = value;
                    } else if (strncmp(s, "SO_MAX_TRANS_GEN_NSEC", 21) == 0) {
                        config->SO_MAX_TRANS_GEN_NSEC = value;
                    } else if (strncmp(s, "SO_RETRY", 8) == 0) {
                        config->SO_RETRY = value;
                    } else if (strncmp(s, "SO_TP_SIZE", 10) == 0) {
                        config->SO_TP_SIZE = value;
                    } else if (strncmp(s, "SO_MIN_TRANS_PROC_NSEC", 22) == 0) {
                        config->SO_MIN_TRANS_PROC_NSEC = value;
                    } else if (strncmp(s, "SO_MAX_TRANS_PROC_NSEC", 22) == 0) {
                        config->SO_MAX_TRANS_PROC_NSEC = value;
                    } else if (strncmp(s, "SO_BUDGET_INIT", 14) == 0) {
                        config->SO_BUDGET_INIT = value;
                    } else if (strncmp(s, "SO_SIM_SEC", 10) == 0) {
                        config->SO_SIM_SEC = value;
                    } else if (strncmp(s, "SO_FRIENDS_NUM", 14) == 0) {
                        config->SO_FRIENDS_NUM = value;
                    } else if (strncmp(s, "SO_HOPS", 7) == 0) {
                        config->SO_HOPS = value;
                    }
                    i++;
            }
        }
    } else {
        printf("File doesn't exists\n");
        EXIT_ON_ERROR
    }

    if (i < 10) {
        printf("Configuration not valid\n");
        kill(0, SIGINT);
    }

    fclose(file);
}

void handler(int signal) {
    union semun sem_ds;
    int sem_value;
    switch (signal) {
        case SIGALRM:

            executing = 0;
            break;

        case SIGINT:
            executing = 0;
            break;

        case SIGTERM:
            executing = 0;
            break;

        case SIGQUIT:
            executing = 0;
            break;

        case SIGUSR1:
            active_users--;
            break;

        case SIGUSR2:
            active_nodes--;
            break;

        default:
            sem_value = semctl(id_sem_writers_block_id, 0, GETVAL, sem_ds.val);
            if (sem_value == 1) {
                unlock(id_sem_writers_block_id);
            }
            break;
    }
}

void cleanIPC() {
    free(node_list);

    shmdt(master_ledger);
    shmdt(user_list);
    shmdt(block_id);
    shmdt(readers_block_id);

    shmctl(id_shm_ledger, IPC_RMID, NULL);
    shmctl(id_shm_user_list, IPC_RMID, NULL);
    shmctl(id_shm_block_id, IPC_RMID, NULL);
    shmctl(id_shm_readers_block_id, IPC_RMID, NULL);

    msgctl(id_msg_node_user, IPC_RMID, NULL);
    msgctl(id_msg_user_node, IPC_RMID, NULL);

    semctl(id_sem_init, 0, IPC_RMID);
    semctl(id_sem_writers_block_id, 0, IPC_RMID);
    semctl(id_sem_readers_block_id, 0, IPC_RMID);
}