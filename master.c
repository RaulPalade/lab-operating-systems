#include "master.h"

configuration (*config);
ledger (*master_ledger);

node_information (*node_info);
user_information (*user_info);

int *readers;
int *last_block_id;     /* Used to read by user till this block */

int id_shared_memory_ledger;            /* Ledger */
int id_shared_memory_configuration;     /* Configuration */
int id_shared_memory_readers;           /* Used to read, not needed because a piece of ledger is written just one time */
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

static int active_nodes = 0;
static int active_users = 0;

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
    int i;
    key_t key;
    struct sigaction sa;
    unsigned short *semval;
    union semun sem_arg;
    struct semid_ds writers_ds;
    time_t current_time;

    struct timespec tp;
    clockid_t clock_id;

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
        raise(SIGQUIT);
    }

    if ((id_shared_memory_ledger = shmget(key, SO_REGISTRY_SIZE * sizeof(block), IPC_CREAT | 0666)) < 0) {
        raise(SIGQUIT);
    }

    if ((void *) (master_ledger = shmat(id_shared_memory_ledger, NULL, 0)) < (void *) 0) {
        raise(SIGQUIT);
    }

    if ((key = ftok("./makefile", 'x')) < 0) {
        raise(SIGQUIT);
    }

    if ((id_shared_memory_configuration = shmget(key, sizeof(configuration), IPC_CREAT | 0666)) < 0) {
        raise(SIGQUIT);
    }

    if ((void *) (config = shmat(id_shared_memory_configuration, NULL, 0)) < (void *) 0) {
        raise(SIGQUIT);
    }
    read_configuration(config);

    if ((key = ftok("./makefile", 'b')) < 0) {
        raise(SIGQUIT);
    }

    if ((id_shared_memory_readers = shmget(key, sizeof(readers), IPC_CREAT | 0666)) < 0) {
        raise(SIGQUIT);
    }

    if ((void *) (readers = shmat(id_shared_memory_readers, NULL, 0)) < (void *) 0) {
        raise(SIGQUIT);
    }
    *readers = 0;

    if ((key = ftok("./makefile", 'y')) < 0) {
        raise(SIGQUIT);
    }

    if ((id_shared_memory_last_block_id = shmget(key, sizeof(last_block_id), IPC_CREAT | 0666)) < 0) {
        raise(SIGQUIT);
    }

    if ((void *) (last_block_id = shmat(id_shared_memory_last_block_id, NULL, 0)) < (void *) 0) {
        raise(SIGQUIT);
    }
    *last_block_id = 0;


    /* NODES AND USERS SHARED MEMORY TO STORE ID AND BALANCE */
    if ((key = ftok("./makefile", 'w')) < 0) {
        raise(SIGQUIT);
    }

    if ((id_shared_memory_nodes = shmget(key, config->SO_NODES_NUM, IPC_CREAT | 0666)) < 0) {
        raise(SIGQUIT);
    }

    if ((void *) (node_info = shmat(id_shared_memory_nodes, NULL, 0)) < (void *) 0) {
        raise(SIGQUIT);
    }

    if ((key = ftok("./makefile", 'z')) < 0) {
        raise(SIGQUIT);
    }

    if ((id_shared_memory_users = shmget(key, config->SO_USERS_NUM, IPC_CREAT | 0666)) < 0) {
        raise(SIGQUIT);
    }

    if ((void *) (user_info = shmat(id_shared_memory_users, NULL, 0)) < (void *) 0) {
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
        switch (fork()) {
            case -1:
                EXIT_ON_ERROR

            case 0:
                active_nodes++;
                node_info[i].pid = getpid();
                node_info[i].balance = 0;
                node_info[i].transactions_left = 0;
                execute_node(i);
        }
    }

    printf("Launching User processes\n");
    for (i = 0; i < (*config).SO_USERS_NUM; i++) {
        switch (fork()) {
            case -1:
                EXIT_ON_ERROR

            case 0:
                active_users++;
                user_info[i].pid = getpid();
                user_info[i].balance = 0;
                execute_user(i);
        }
    }

    unlock_init_semaphore(id_semaphore_init);

    printf("Starting timer right now\n");
    alarm((*config).SO_SIM_SEC);

    clock_gettime(clock_id, &tp);
    srand(tp.tv_sec);
    current_time = time(NULL);
    while (executing && active_users > 0 && !ledger_full) {
        if ((time(NULL) - current_time) >= 1) {
            print_node_info();
            print_user_info();
            current_time = time(NULL);
        }
    }

    while (wait(NULL) > 0) {}
    nanosleep(&interval, NULL);

    /* VARIOUSE READS FROM QUEUE */

    print_final_report();

    shmdt(master_ledger);
    shmdt(readers);
    shmdt(config);
    shmdt(last_block_id);
    shmdt(node_info);
    shmdt(user_info);
    shmctl(id_shared_memory_ledger, IPC_RMID, NULL);
    shmctl(id_shared_memory_readers, IPC_RMID, NULL);
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
    free(semval);
    return 0;
}

void execute_node(int arg) {
    char *argv[3];
    argv[0] = "node";
    sprintf(argv[1], "%d", arg);
    argv[2] = NULL;
    execv("node", argv);
}

void execute_user(int arg) {
    char *argv[3];
    argv[0] = "user";
    sprintf(argv[1], "%d", arg);
    argv[2] = NULL;
    execv("user", argv);
}

void handler(int signal) {
    ledger_full = 1;
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
        printf("Node PID=%d, Balance=%d\n", node_info[i].pid, node_info[i].balance);
    }
}

void print_final_report() {
    printf("Simulation ended with flags\n");
    printf("Executing = %d\n", executing);
    printf("Active users = %d\n", active_users);
    printf("Ledger full = %d\n", ledger_full);
    print_user_info();
    print_node_info();
    printf("User processes dead = %d\n", config->SO_USERS_NUM - active_users);
    printf("Number of blocks in the ledger = %d\n", *last_block_id);
}