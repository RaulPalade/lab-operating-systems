#include "master.h"
#include "util.h"

configuration (*config);
ledger (*master_ledger);

node_information (*node_info);
user_information (*user_info);

int *last_block_id;     /* Used to read by user till this block */

int id_shared_memory_ledger;            /* Ledger */
int id_shared_memory_configuration;     /* Configuration */
int id_shared_memory_last_block_id;          /* Used to keep trace of block_id for next block and to read operations */
int id_shared_memory_nodes;
int id_shared_memory_users;

int id_message_queue_master_node;
int id_message_queue_master_user;
int id_message_queue_node_user;

int id_semaphore_init;
int id_semaphore_writers;
int id_semaphore_mutex;

volatile int executing = 1;
int ledger_full = 0;
int dead_users = 0;

static int active_nodes = 0;
static int active_users = 0;

typedef struct {
    long mtype;
    char letter;
} chat_message;


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
int main() {
    chat_message cm;
    struct msqid_ds buf;
    configuration c;
    int remaining_seconds;
    pid_t node_pid;
    pid_t user_pid;
    int i;
    key_t key;
    struct sigaction sa;
    union semun sem_arg;
    struct semid_ds writers_ds;
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
    if ((key = ftok("./makefile", 'a')) < 0) {
        EXIT_ON_ERROR
        raise(SIGQUIT);
    }

    if ((id_shared_memory_ledger = shmget(key, SO_REGISTRY_SIZE * sizeof(block), IPC_CREAT | 0666)) < 0) {
        EXIT_ON_ERROR
        raise(SIGQUIT);
    }

    if ((void *) (master_ledger = shmat(id_shared_memory_ledger, NULL, 0)) < (void *) 0) {
        EXIT_ON_ERROR
        raise(SIGQUIT);
    }

    if ((key = ftok("./makefile", 'x')) < 0) {
        EXIT_ON_ERROR
        raise(SIGQUIT);
    }

    if ((id_shared_memory_configuration = shmget(key, sizeof(configuration), IPC_CREAT | 0666)) < 0) {
        EXIT_ON_ERROR
        raise(SIGQUIT);
    }

    if ((void *) (config = shmat(id_shared_memory_configuration, NULL, 0)) < (void *) 0) {
        EXIT_ON_ERROR
        raise(SIGQUIT);
    }
    *config = read_configuration();

    if ((key = ftok("./makefile", 'y')) < 0) {
        EXIT_ON_ERROR
        raise(SIGQUIT);
    }

    if ((id_shared_memory_last_block_id = shmget(key, sizeof(last_block_id), IPC_CREAT | 0666)) < 0) {
        EXIT_ON_ERROR
        raise(SIGQUIT);
    }

    if ((void *) (last_block_id = shmat(id_shared_memory_last_block_id, NULL, 0)) < (void *) 0) {
        EXIT_ON_ERROR
        raise(SIGQUIT);
    }
    *last_block_id = 0;


    /* NODES AND USERS SHARED MEMORY TO STORE ID AND BALANCE */
    if ((key = ftok("./makefile", 'w')) < 0) {
        EXIT_ON_ERROR
        raise(SIGQUIT);
    }

    if ((id_shared_memory_nodes = shmget(key, config->SO_NODES_NUM, IPC_CREAT | 0666)) < 0) {
        EXIT_ON_ERROR
        raise(SIGQUIT);
    }

    if ((void *) (node_info = shmat(id_shared_memory_nodes, NULL, 0)) < (void *) 0) {
        EXIT_ON_ERROR
        raise(SIGQUIT);
    }

    if ((key = ftok("./makefile", 'z')) < 0) {
        EXIT_ON_ERROR
        raise(SIGQUIT);
    }

    if ((id_shared_memory_users = shmget(key, config->SO_USERS_NUM, IPC_CREAT | 0666)) < 0) {
        EXIT_ON_ERROR
        raise(SIGQUIT);
    }

    if ((void *) (user_info = shmat(id_shared_memory_users, NULL, 0)) < (void *) 0) {
        EXIT_ON_ERROR
        raise(SIGQUIT);
    }

    /* MESSAGE QUEUQ CREATION */
    id_message_queue_master_node = new_msg_queue_id('c');
    id_message_queue_master_user = new_msg_queue_id('d');
    id_message_queue_node_user = new_msg_queue_id('e');

    /* SEMAPHORE CREATION */
    if ((key = ftok("./makefile", 'f')) < 0) {
        raise(SIGQUIT);
    }

    if ((id_semaphore_init = semget(key, 1, IPC_CREAT | 0666)) < 0) {
        raise(SIGQUIT);
    }

    /* INIZIALIZZAZIONE SEMAFORO A 1 PER LA CREAZIONE DELLE RISORSE */
    sem_arg.val = 1;
    if (semctl(id_semaphore_init, 0, SETVAL, sem_arg)) {
        EXIT_ON_ERROR
    }

    if ((key = ftok("./makefile", 'g')) < 0) {
        raise(SIGQUIT);
    }

    if ((id_semaphore_writers = semget(key, 1, IPC_CREAT | 0666)) < 0) {
        raise(SIGQUIT);
    }

    sem_arg.val = 1;
    if (semctl(id_semaphore_writers, 0, SETVAL, sem_arg)) {
        EXIT_ON_ERROR
    }

    if ((key = ftok("./makefile", 'h')) < 0) {
        raise(SIGQUIT);
    }

    if ((id_semaphore_mutex = semget(key, 1, IPC_CREAT | 0666)) < 0) {
        raise(SIGQUIT);
    }

    sem_arg.val = 1;
    if (semctl(id_semaphore_mutex, 0, SETVAL, sem_arg)) {
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
                active_nodes++;
                node_info[i].pid = node_pid;
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
                user_info[i].pid = user_pid;
        }
    }

    printf("Starting timer right now\n");
    remaining_seconds = config->SO_SIM_SEC;
    alarm(config->SO_SIM_SEC);
    unlock_init_semaphore(id_semaphore_init);
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
    
    /* while (wait(NULL) > 0) {} */

    /* for (i = 0; i < config->SO_USERS_NUM; i++) {
        kill(user_info[i].pid, SIGKILL);
    }

    for (i = 0; i < config->SO_NODES_NUM; i++) {
        kill(node_info[i].pid, SIGKILL);
    } */

    shmdt(master_ledger);
    shmdt(config);
    shmdt(last_block_id);
    shmdt(node_info);
    shmdt(user_info);
    shmctl(id_shared_memory_ledger, IPC_RMID, NULL);
    shmctl(id_shared_memory_configuration, IPC_RMID, NULL);
    shmctl(id_shared_memory_last_block_id, IPC_RMID, NULL);
    shmctl(id_shared_memory_nodes, IPC_RMID, NULL);
    shmctl(id_shared_memory_users, IPC_RMID, NULL);

    msgctl(id_message_queue_master_node, IPC_RMID, NULL);
    msgctl(id_message_queue_master_user, IPC_RMID, NULL);
    msgctl(id_message_queue_node_user, IPC_RMID, NULL);

    semctl(id_semaphore_init, 0, IPC_RMID);
    semctl(id_semaphore_writers, 0, IPC_RMID);
    semctl(id_semaphore_mutex, 0, IPC_RMID);

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

int new_msg_queue_id(char proj_id) {
    key_t key;
    int id;

    if ((key = ftok("./makefile", proj_id)) < 0) {
        raise(SIGQUIT);
    }

    if ((id = msgget(key, IPC_CREAT | 0666)) < 0) {
        raise(SIGQUIT);
    }

    return id;
}

void print_live_info() {
    int i;
    printf("-----------------------------------------------------------------------------------------------------\n");
    printf("%10s %10s\n", "ACTIVE NODES", "ACTIVE USERS");
    printf("%10d %10d\n", active_nodes, active_users);
    print_node_info();
    print_user_info();
}

void print_node_info() {
    int i;
    for (i = 0; i < config->SO_NODES_NUM; i++) {
        printf("Node PID=%d, Balance=%d, Transaction Left=%d\n", node_info[i].pid, node_info[i].balance,
               node_info[i].transactions_left);
    }
}

void print_user_info() {
    int i;
    for (i = 0; i < config->SO_USERS_NUM; i++) {
        printf("User PID=%d, Balance=%d\n", user_info[i].pid, user_info[i].balance);
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
                        printf("budget init = %d\n", config.SO_BUDGET_INIT);
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
            shmdt(master_ledger);
            shmdt(config);
            shmdt(last_block_id);
            shmdt(node_info);
            shmdt(user_info);
            shmctl(id_shared_memory_ledger, IPC_RMID, NULL);
            shmctl(id_shared_memory_configuration, IPC_RMID, NULL);
            shmctl(id_shared_memory_last_block_id, IPC_RMID, NULL);
            shmctl(id_shared_memory_nodes, IPC_RMID, NULL);
            shmctl(id_shared_memory_users, IPC_RMID, NULL);

            msgctl(id_message_queue_master_node, IPC_RMID, NULL);
            msgctl(id_message_queue_master_user, IPC_RMID, NULL);
            msgctl(id_message_queue_node_user, IPC_RMID, NULL);

            semctl(id_semaphore_init, 0, IPC_RMID);
            semctl(id_semaphore_writers, 0, IPC_RMID);
            semctl(id_semaphore_mutex, 0, IPC_RMID);
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