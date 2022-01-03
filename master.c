#include "master.h"
#include "util.h"

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
int id_shm_user_list;
int id_shm_last_block_id;
int id_shm_ledger_size;

/* MESSAGE QUEUE */
int id_msg_node_user;
int id_msg_user_node;

/* SEMAPHORE*/
int id_sem_init;
int id_sem_writers;

/* SIMULATION RUNNING PARAMETERS */
volatile int executing = 1;
int ledger_full = 0;
int active_users = 0;

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
    int i;
    int remaining_seconds;
    pid_t node_pid;
    pid_t user_pid;
    struct sigaction sa;
    union semun sem_arg;
    struct timespec interval;
    interval.tv_sec = 1;
    interval.tv_nsec = 0;

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
    *config = read_configuration();
    master_ledger = new_shared_memory(PROJ_ID_SHM_LEDGER, id_shm_ledger, sizeof(ledger));
    node_list = new_shared_memory(PROJ_ID_SHM_NODE_LIST, id_shm_node_list,
                                  sizeof(node_information) * config->SO_NODES_NUM);
    user_list = new_shared_memory(PROJ_ID_SHM_USER_LIST, id_shm_user_list,
                                  sizeof(user_information) * config->SO_USERS_NUM);
    last_block_id = new_shared_memory(PROJ_ID_SHM_LAST_BLOCK_ID, id_shm_last_block_id, sizeof(last_block_id));
    *last_block_id = 0;
    ledger_size = new_shared_memory(PROJ_ID_SHM_LEDGER_SIZE, id_shm_ledger, sizeof(ledger_size));
    *ledger_size = 0;

    /* MESSAGE QUEUE CREATION */
    id_msg_node_user = new_message_queue(PROJ_ID_MSG_NODE_USER);
    id_msg_user_node = new_message_queue(PROJ_ID_MSG_USER_NODE);

    /* SEMAPHORE CREATION */
    id_sem_init = new_semaphore(PROJ_ID_SEM_INIT);
    sem_arg.val = 1;
    if (semctl(id_sem_init, 0, SETVAL, sem_arg)) {
        EXIT_ON_ERROR
    }

    id_sem_writers = new_semaphore(PROJ_ID_SEM_WRITERS);
    sem_arg.val = 1;
    if (semctl(id_sem_writers, 0, SETVAL, sem_arg)) {
        EXIT_ON_ERROR
    }

    printf("Launching Node processes\n");
    for (i = 0; i < (*config).SO_NODES_NUM; i++) {
        switch (node_pid = fork()) {
            case -1:
                EXIT_ON_ERROR

            case 0:
                execute_node(i);

            default:
                node_list[i].pid = node_pid;
        }
    }

    printf("Launching User processes\n");
    for (i = 0; i < (*config).SO_USERS_NUM; i++) {
        switch (user_pid = fork()) {
            case -1:
                EXIT_ON_ERROR

            case 0:
                execute_user(i);

            default:
                active_users++;
                user_list[i].pid = user_pid;
        }
    }

    printf("Starting timer right now\n");
    remaining_seconds = config->SO_SIM_SEC;
    alarm(config->SO_SIM_SEC);
    unlock_init_semaphore(id_sem_init);
    while (executing && !ledger_full && active_users > 0) {
        /* print_ledger(master_ledger); */
        print_node_info();
        print_user_info();
        printf("Remaining seconds = %d\n", remaining_seconds--);
        nanosleep(&interval, NULL);
    }

    /* kill(0, SIGQUIT); */
    print_final_report();
    print_ledger(master_ledger);

    /*while (wait(NULL) > 0) {}

    for (i = 0; i < config->SO_USERS_NUM; i++) {
        kill(user_list[i].pid, SIGKILL);
    }

    for (i = 0; i < config->SO_NODES_NUM; i++) {
        kill(node_list[i].pid, SIGKILL);
    }*/

    shmdt(config);
    shmdt(master_ledger);
    shmdt(node_list);
    shmdt(user_list);
    shmdt(last_block_id);
    shmdt(ledger_size);

    shmctl(id_shm_configuration, IPC_RMID, NULL);
    shmctl(id_shm_ledger, IPC_RMID, NULL);
    shmctl(id_shm_node_list, IPC_RMID, NULL);
    shmctl(id_shm_user_list, IPC_RMID, NULL);
    shmctl(id_shm_last_block_id, IPC_RMID, NULL);
    shmctl(id_shm_ledger_size, IPC_RMID, NULL);

    msgctl(id_msg_node_user, IPC_RMID, NULL);
    msgctl(id_msg_user_node, IPC_RMID, NULL);

    semctl(id_sem_init, 0, IPC_RMID);
    semctl(id_sem_writers, 0, IPC_RMID);

    return 0;
}

void execute_node(int index) {
    char *argv[3];
    char array[2];
    sprintf(array, "%d", index);
    argv[0] = "node";
    argv[1] = array;
    argv[2] = NULL;
    execv("node", argv);
}

void execute_user(int index) {
    char *argv[3];
    char array[2];
    sprintf(array, "%d", index);
    argv[0] = "user";
    argv[1] = array;
    argv[2] = NULL;
    execv("user", argv);
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
        printf("Node PID=%d, Balance=%d, Transaction Left=%d\n", user_list[i].pid, node_list[i].balance,
               node_list[i].transactions_left);
    }
}

void print_user_info() {
    int i;
    for (i = 0; i < config->SO_USERS_NUM; i++) {
        printf("User PID=%d, Balance=%d\n", user_list[i].pid, user_list[i].balance);
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
    printf("Number of blocks in the ledger = %d\n", *last_block_id);
    printf("---------------------END---------------------\n");
}

void print_ledger(ledger *ledger) {
    int i;
    printf("Printing ledger\n");
    for (i = 0; i < *ledger_size; i++) {
        print_block(ledger->blocks[i]);
        printf("-----------------------------------------------------------------------------------------------------\n");
    }
    printf("-----------------------------------------------------------------------------------------------------\n");
}

configuration read_configuration() {
    configuration config;
    FILE *file;
    char s[23];
    char comment;
    char filename[] = "configuration1.conf";
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
        EXIT_ON_ERROR;
    }

    if (i < 10) {
        printf("Configuration not valid\n");
        kill(0, SIGINT);
    }

    fflush(file);
    fclose(file);

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
            print_final_report();
            shmdt(config);
            shmdt(master_ledger);
            shmdt(node_list);
            shmdt(user_list);
            shmdt(last_block_id);
            shmdt(ledger_size);

            shmctl(id_shm_configuration, IPC_RMID, NULL);
            shmctl(id_shm_ledger, IPC_RMID, NULL);
            shmctl(id_shm_node_list, IPC_RMID, NULL);
            shmctl(id_shm_user_list, IPC_RMID, NULL);
            shmctl(id_shm_last_block_id, IPC_RMID, NULL);
            shmctl(id_shm_ledger_size, IPC_RMID, NULL);

            msgctl(id_msg_node_user, IPC_RMID, NULL);
            msgctl(id_msg_user_node, IPC_RMID, NULL);

            semctl(id_sem_init, 0, IPC_RMID);
            semctl(id_sem_writers, 0, IPC_RMID);
            exit(0);

        case SIGUSR1:
            printf("Master received SIGUSR1\n");
            active_users--;
            break;

        case SIGUSR2:
            printf("Master received SIGUSR2\n");
            ledger_full = 1;
            break;

        default:
            break;
    }
}