#include "../include/master.h"

configuration (*config);
ledger (*master_ledger);
node_information (*node_list);
user_information (*user_list);
int *block_id;
int *readers_block_id;

/* SHARED MEMORY */
int id_shm_configuration;
int id_shm_ledger;
int id_shm_node_list;
int id_shm_user_list;
int id_shm_block_id;
int id_shm_readers_block_id;

/* MESSAGE QUEUE */
int id_msg_node_user;
int id_msg_user_node;

/* SEMAPHORE*/
int id_sem_init;
int id_sem_writers_block_id;
int id_sem_readers_block_id;

/* SIMULATION RUNNING PARAMETERS */
volatile int executing = 1;
int ledger_full = 0;
int active_nodes = 0;
int active_users = 0;

unsigned long initial_total_funds;
unsigned long final_total_funds;

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
    char *args_node[13] = {NODE};
    char *args_user[14] = {USER};
    char index_node[3 * sizeof(int) + 1];
    char index_user[3 * sizeof(int) + 1];
    char id_shm_configuration_str[100 * sizeof(int) + 1];
    char id_shm_ledger_str[3 * sizeof(int) + 1];
    char id_shm_node_list_str[3 * sizeof(int) + 1];
    char id_shm_user_list_str[3 * sizeof(int) + 1];
    char id_shm_block_id_str[3 * sizeof(int) + 1];
    char is_shm_readers_block_id[3 * sizeof(int) + 1];
    char id_msg_node_user_str[3 * sizeof(int) + 1];
    char id_msg_user_node_str[3 * sizeof(int) + 1];
    char id_sem_init_str[3 * sizeof(int) + 1];
    char id_sem_writers_block_id_str[3 * sizeof(int) + 1];
    char id_sem_readers_block_id_str[3 * sizeof(int) + 1];
    key_t key;
    int i;
    int remaining_seconds;
    pid_t node_pid;
    pid_t user_pid;
    struct sigaction sa;
    struct timespec interval;
    interval.tv_sec = 1;
    interval.tv_nsec = 0;

    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handler;
    sigaction(SIGALRM, &sa, 0);
    sigaction(SIGINT, &sa, 0);
    sigaction(SIGTERM, &sa, 0);
    sigaction(SIGQUIT, &sa, 0);
    sigaction(SIGUSR1, &sa, 0);
    sigaction(SIGUSR2, &sa, 0);

    /* SHARED MEMORY CREATION */
    key = ftok("../makefile", PROJ_ID_SHM_CONFIGURATION);
    EXIT_ON_ERROR
    id_shm_configuration = shmget(key, sizeof(configuration), IPC_CREAT | 0666);
    EXIT_ON_ERROR
    config = shmat(id_shm_configuration, NULL, 0);
    EXIT_ON_ERROR
    *config = read_configuration();

    key = ftok("../makefile", PROJ_ID_SHM_LEDGER);
    EXIT_ON_ERROR
    id_shm_ledger = shmget(key, sizeof(master_ledger), IPC_CREAT | 0666);
    EXIT_ON_ERROR
    master_ledger = shmat(id_shm_ledger, NULL, 0);
    EXIT_ON_ERROR

    key = ftok("../makefile", PROJ_ID_SHM_NODE_LIST);
    EXIT_ON_ERROR
    id_shm_node_list = shmget(key, sizeof(node_information) * config->SO_NODES_NUM, IPC_CREAT | 0666);
    EXIT_ON_ERROR
    node_list = shmat(id_shm_node_list, NULL, 0);
    EXIT_ON_ERROR

    key = ftok("../makefile", PROJ_ID_SHM_USER_LIST);
    EXIT_ON_ERROR
    id_shm_user_list = shmget(key, sizeof(user_information) * config->SO_USERS_NUM, IPC_CREAT | 0666);
    EXIT_ON_ERROR
    user_list = shmat(id_shm_user_list, NULL, 0);
    EXIT_ON_ERROR

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

    /* Preparing cmd line arguments for execv */
    sprintf(id_shm_configuration_str, "%d", id_shm_configuration);
    sprintf(id_shm_ledger_str, "%d", id_shm_ledger);
    sprintf(id_shm_node_list_str, "%d", id_shm_node_list);
    sprintf(id_shm_user_list_str, "%d", id_shm_user_list);
    sprintf(id_shm_block_id_str, "%d", id_shm_block_id);
    sprintf(is_shm_readers_block_id, "%d", id_shm_readers_block_id);

    sprintf(id_msg_node_user_str, "%d", id_msg_node_user);
    sprintf(id_msg_user_node_str, "%d", id_msg_user_node);

    sprintf(id_sem_init_str, "%d", id_sem_init);
    sprintf(id_sem_writers_block_id_str, "%d", id_sem_writers_block_id);
    sprintf(id_sem_readers_block_id_str, "%d", id_sem_readers_block_id);

    args_node[2] = id_shm_configuration_str;
    args_node[3] = id_shm_ledger_str;
    args_node[4] = id_shm_node_list_str;
    args_node[5] = id_shm_block_id_str;
    args_node[6] = is_shm_readers_block_id;
    args_node[7] = id_msg_node_user_str;
    args_node[8] = id_msg_user_node_str;
    args_node[9] = id_sem_init_str;
    args_node[10] = id_sem_writers_block_id_str;
    args_node[11] = id_sem_readers_block_id_str;
    args_node[12] = NULL;

    args_user[2] = id_shm_configuration_str;
    args_user[3] = id_shm_ledger_str;
    args_user[4] = id_shm_node_list_str;
    args_user[5] = id_shm_user_list_str;
    args_user[6] = id_shm_block_id_str;
    args_user[7] = is_shm_readers_block_id;

    args_user[8] = id_msg_node_user_str;
    args_user[9] = id_msg_user_node_str;
    args_user[10] = id_sem_init_str;
    args_user[11] = id_sem_writers_block_id_str;
    args_user[12] = id_sem_readers_block_id_str;
    args_user[13] = NULL;

    printf("Launching Node processes\n");
    for (i = 0; i < (*config).SO_NODES_NUM; i++) {
        switch (node_pid = fork()) {
            case -1:
                EXIT_ON_ERROR

            case 0:
                sprintf(index_node, "%d", i);
                args_node[1] = index_node;
                execv("node", args_node);

            default:
                active_nodes++;
                node_list[i].pid = node_pid;
        }
    }

    printf("Launching User processes\n");
    for (i = 0; i < (*config).SO_USERS_NUM; i++) {
        switch (user_pid = fork()) {
            case -1:
                EXIT_ON_ERROR

            case 0:
                sprintf(index_user, "%d", i);
                args_user[1] = index_user;
                execv("user", args_user);

            default:
                initial_total_funds += 1000;
                active_users++;
                user_list[i].pid = user_pid;
        }
    }

    printf("Starting timer right now\n");
    remaining_seconds = config->SO_SIM_SEC;
    alarm(config->SO_SIM_SEC);
    unlock_init_semaphore(id_sem_init);
    while (executing && !ledger_full && active_users > 0) {
        /* print_node_info();
         print_user_info();*/
        printf("Remaining seconds = %d\n", remaining_seconds--);
        printf("config->SO_NODES_NUM = %d\n", config->SO_NODES_NUM);
        printf("config->SO_USERS_NUM = %d\n", config->SO_USERS_NUM);
        nanosleep(&interval, NULL);

    }

    kill(0, SIGTERM);
    /*print_final_report();*/
    print_ledger(master_ledger);

    for (i = 0; i < 10; i++) {
        final_total_funds += node_list[i].balance;
    }

    for (i = 0; i < 100; i++) {
        final_total_funds += user_list[i].balance;
    }

    printf("config->SO_NODES_NUM = %d\n", config->SO_NODES_NUM);
    printf("config->SO_USERS_NUM = %d\n", config->SO_USERS_NUM);

    cleanIPC();

    printf("Initial total funds = %ld\n", initial_total_funds);
    printf("Final total funds = %ld\n", final_total_funds);
    /* assert(initial_total_funds == final_total_funds);*/

    return 0;
}

void print_ledger(ledger *ledger) {
    int i;
    printf("Printing ledger\n");
    for (i = 0; i < *block_id; i++) {
        print_block(ledger->blocks[i]);
        printf("-----------------------------------------------------------------------------------------------------\n");
    }
    printf("-----------------------------------------------------------------------------------------------------\n");
}

void print_live_info() {
    printf("-----------------------------------------------------------------------------------------------------\n");
    printf("%10s\n", "ACTIVE USERS");
    printf("%10d\n", active_users);
    print_node_info();
    print_user_info();
}

void print_node_info() {
    int i;
    for (i = 0; i < config->SO_NODES_NUM; i++) {
        printf("Node PID=%d, Balance=%ld, Transaction Left=%ld\n", user_list[i].pid, node_list[i].balance,
               node_list[i].transactions_left);
    }
}

void print_user_info() {
    int i;
    for (i = 0; i < config->SO_USERS_NUM; i++) {
        printf("User PID=%d, Balance=%ld\n", user_list[i].pid, user_list[i].balance);
    }
}

void print_final_report() {
    printf("\n---------------------END---------------------\n");
    printf("Simulation ended with flags\n");
    printf("Executing = %d\n", executing);
    printf("Active users = %d\n", active_users);
    printf("Ledger full = %d\n", ledger_full);
    print_node_info();
    print_user_info();
    printf("User processes dead = %d\n", config->SO_USERS_NUM - active_users);
    printf("Number of blocks in the ledger = %d\n", *block_id);
    printf("---------------------END---------------------\n");
}

configuration read_configuration() {
    configuration config;
    FILE *file;
    char s[23];
    char comment;
    char filename[] = "../configurations/configuration1.conf";
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
                        config.SO_USERS_NUM = value;
                    } else if (strncmp(s, "SO_NODES_NUM", 12) == 0) {
                        config.SO_NODES_NUM = value;
                    } else if (strncmp(s, "SO_REWARD", 9) == 0) {
                        config.SO_REWARD = value;
                    } else if (strncmp(s, "SO_MIN_TRANS_GEN_NSEC", 21) == 0) {
                        config.SO_MIN_TRANS_GEN_NSEC = value;
                    } else if (strncmp(s, "SO_MAX_TRANS_GEN_NSEC", 21) == 0) {
                        config.SO_MAX_TRANS_GEN_NSEC = value;
                    } else if (strncmp(s, "SO_RETRY", 8) == 0) {
                        config.SO_RETRY = value;
                    } else if (strncmp(s, "SO_TP_SIZE", 10) == 0) {
                        config.SO_TP_SIZE = value;
                    } else if (strncmp(s, "SO_MIN_TRANS_PROC_NSEC", 22) == 0) {
                        config.SO_MIN_TRANS_PROC_NSEC = value;
                    } else if (strncmp(s, "SO_MAX_TRANS_PROC_NSEC", 22) == 0) {
                        config.SO_MAX_TRANS_PROC_NSEC = value;
                    } else if (strncmp(s, "SO_BUDGET_INIT", 14) == 0) {
                        config.SO_BUDGET_INIT = value;
                    } else if (strncmp(s, "SO_SIM_SEC", 10) == 0) {
                        config.SO_SIM_SEC = value;
                    } else if (strncmp(s, "SO_FRIENDS_NUM", 14) == 0) {
                        config.SO_FRIENDS_NUM = value;
                    } else if (strncmp(s, "SO_HOPS", 7) == 0) {
                        config.SO_HOPS = value;
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

    fflush(file);
    fclose(file);

    if (config.SO_TP_SIZE == 20) {
        printf(ANSI_COLOR_RED "NON FUNZIONA STA CAZZO DI CONFIGURAZIONE\n" ANSI_COLOR_RESET);
    }

    return config;
}

void handler(int signal) {
    switch (signal) {
        case SIGALRM:
            printf("Timer expired\n");
            executing = 0;
            break;

        case SIGINT:
            printf("Master received SIGINT\n");
            executing = 0;
            break;

        case SIGTERM:
            printf("Master received SIGTERM\n");
            executing = 0;
            break;

        case SIGQUIT:
            printf("Master received SIGQUIT\n");
            executing = 0;
            break;

        case SIGUSR1:
            printf("Master received SIGUSR1\n");
            active_users--;
            /* Rimuovere utente dalla lista degli utenti */
            break;

        case SIGUSR2:
            printf("Master received SIGUSR2\n");
            active_nodes--;
            break;

        default:
            break;
    }
}

void cleanIPC() {
    shmdt(config);
    shmdt(master_ledger);
    shmdt(node_list);
    shmdt(user_list);
    shmdt(block_id);
    shmdt(readers_block_id);

    shmctl(id_shm_configuration, IPC_RMID, NULL);
    shmctl(id_shm_ledger, IPC_RMID, NULL);
    shmctl(id_shm_node_list, IPC_RMID, NULL);
    shmctl(id_shm_user_list, IPC_RMID, NULL);
    shmctl(id_shm_block_id, IPC_RMID, NULL);
    shmctl(id_shm_readers_block_id, IPC_RMID, NULL);

    msgctl(id_msg_node_user, IPC_RMID, NULL);
    msgctl(id_msg_user_node, IPC_RMID, NULL);

    semctl(id_sem_init, 0, IPC_RMID);
    semctl(id_sem_writers_block_id, 0, IPC_RMID);
    semctl(id_sem_readers_block_id, 0, IPC_RMID);
}
