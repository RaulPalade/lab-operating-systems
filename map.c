#include "map.h"
#include "general.h"
#include "source.h"

simulation_configuration configuration;
int id_shared_memory_sources;
int id_shared_memory_map;
int id_shared_memory_ex;
int id_shared_memory_readers;
int id_message_queue_source_taxi;
int id_message_queue_master_map_source;
int writers;
int semaphore;
int mutex;
int executing = 1;
map_point (*source_list_ptr)[];
cell (*map_ptr)[][SO_HEIGHT];
int *readers;
int dead_taxis = 0;
master_message top_cells;

int main(int argc, char **argv) {
    int i;
    int count;
    key_t key;
    int row;
    int col;
    union semun sem_arg;
    union semun sem_arg1;
    unsigned short semval[SO_WIDTH * SO_HEIGHT];
    struct sigaction sa;
    struct semid_ds semid_ds;
    struct semid_ds writers_ds;
    struct semid_ds source_ds;
    struct sembuf buf;
    struct msqid_ds qds;

    printf("==========\n INIT MAP\n==========\n");

    /* INIT */
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handler;
    sigaction(SIGINT, &sa, 0);
    sigaction(SIGALRM, &sa, 0);
    sigaction(SIGQUIT, &sa, 0);
    sigaction(SIGUSR1, &sa, 0);
    sigaction(SIGUSR2, &sa, 0);
    sigaction(SIGTSTP, &sa, 0);

    /* SHARED MEMORY MANAGEMENT */
    if ((key = ftok("./makefile", 'm')) < 0) {
        log_message("Error: ftok", RUNTIME);
        EXIT_ON_ERROR
    }

    if ((id_shared_memory_map = shmget(key, 0, 0666)) < 0) {
        log_message("Error: shmget", RUNTIME);
        EXIT_ON_ERROR
    } else {
        printf("id_shared_memory_map=%d\n", id_shared_memory_map);
    }

    if ((void *) (map_ptr = shmat(id_shared_memory_map, NULL, 0)) < (void *) 0) {
        log_message("Error: shmat", RUNTIME);
        EXIT_ON_ERROR
    }

    if ((key = ftok("./makefile", 'b')) < 0) {
        log_message("Error: ftok", RUNTIME);
        EXIT_ON_ERROR
    }

    if ((id_shared_memory_sources = shmget(key, SO_WIDTH * SO_HEIGHT * sizeof(map_point), IPC_CREAT | 0644)) < 0) {
        log_message("Error: shmget", RUNTIME);
        EXIT_ON_ERROR
    } else {
        printf("id_shared_memory_sources=%d\n", id_shared_memory_sources);
    }

    if ((void *) (source_list_ptr = shmat(id_shared_memory_sources, NULL, 0)) < (void *) 0) {
        log_message("Error: shmat", RUNTIME);
        EXIT_ON_ERROR
    }

    if ((key = ftok("./makefile", 'z')) < 0) {
        log_message("Error: ftok", RUNTIME);
        EXIT_ON_ERROR
    }

    if ((id_shared_memory_readers = shmget(key, SO_WIDTH * SO_HEIGHT * sizeof(map_point), IPC_CREAT | 0666)) < 0) {
        log_message("Error: shmget", RUNTIME);
        EXIT_ON_ERROR
    } else {
        printf("id_shared_memory_readers=%d\n", id_shared_memory_readers);
    }

    if ((void *) (readers = shmat(id_shared_memory_sources, NULL, 0)) < (void *) 0) {
        log_message("Error: shmat", RUNTIME);
        EXIT_ON_ERROR
    }
    *readers = 0;

    /* MESSAGE QUEUE MANAGEMENT */
    if ((key = ftok("./makefile", 's')) < 0) {
        log_message("Error: ftok", RUNTIME);
        EXIT_ON_ERROR
    }

    if ((id_message_queue_master_map_source = msgget(key, 0666)) < 0) {
        log_message("Error: msgget", RUNTIME);
        EXIT_ON_ERROR
    } else {
        printf("id_message_queue_master_map_source=%d\n", id_message_queue_master_map_source);
    }

    if ((key = ftok("./makefile", 'q')) < 0) {
        log_message("Error: ftok", RUNTIME);
        EXIT_ON_ERROR
    }

    if ((id_message_queue_source_taxi = msgget(key, IPC_CREAT | 0666)) < 0) {
        log_message("Error: msgget", RUNTIME);
        EXIT_ON_ERROR
    } else {
        printf("id_message_queue_source_taxi=%d\n", id_message_queue_source_taxi);
    }

    /* SEMAPHORE MANAGEMENT */
    if ((key = ftok("./makefile", 'w')) < 0) {
        log_message("Error: ftok", RUNTIME);
        EXIT_ON_ERROR
    }

    if ((writers = semget(key, SO_WIDTH * SO_HEIGHT, IPC_CREAT | 0666)) < 0) {
        log_message("Error: semget", RUNTIME);
        EXIT_ON_ERROR
    } else {
        printf("writers=%d\n", writers);
    }

    sem_arg.buf = &writers_ds;
    for (count = 0; count < SO_WIDTH * SO_HEIGHT; count++) {
        semval[count] = 1;
    }
    sem_arg.array = semval;
    if (semctl(writers, 0, SETALL, sem_arg) < 0) {
        EXIT_ON_ERROR
    }

    if ((key = ftok("./makefile", 'm')) < 0) {
        log_message("Error: ftok", RUNTIME);
        EXIT_ON_ERROR
    }

    if ((mutex = semget(key, 1, IPC_CREAT | 0666)) < 0) {
        log_message("Error: semget", RUNTIME);
        EXIT_ON_ERROR
    } else {
        printf("mutex=%d\n", mutex);
    }
    sem_arg.val = 1;
    if (semctl(mutex, 0, SETVAL, sem_arg) < 0) {
        EXIT_ON_ERROR
    }

    if ((key = ftok("./makefile", 'y')) < 0) {
        log_message("Error: ftok", RUNTIME);
        EXIT_ON_ERROR
    }

    if ((semaphore = semget(key, 1, IPC_CREAT | 0666)) < 0) {
        log_message("Error: semget", RUNTIME);
        EXIT_ON_ERROR
    } else {
        printf("semaphore=%d\n", semaphore);
    }
    sem_arg.val = 1;
    if (semctl(semaphore, 0, SETVAL, sem_arg) < 0) {
        EXIT_ON_ERROR
    }

    shmdt(map_ptr);
    shmdt(source_list_ptr);
    shmdt(readers);

    shmctl(id_shared_memory_sources, IPC_RMID, NULL);
    shmctl(id_shared_memory_readers, IPC_RMID, NULL);

    msgctl(id_message_queue_source_taxi, IPC_RMID, NULL);

    semctl(writers, 0, IPC_RMID);
    semctl(mutex, 0, IPC_RMID);
    semctl(semaphore, 0, IPC_RMID);

    exit(EXIT_SUCCESS);
}

void unblock(int semaphore) {

}

void parse_configuration(simulation_configuration *configuration) {

}

int check_no_adjacent_holes(cell (*matrix)[][SO_HEIGHT], int x, int y) {
    return 1;
}

void generate_map(cell (*matrix)[][SO_HEIGHT], simulation_configuration *configuration) {

}

void print_map(cell (*matrix)[][SO_HEIGHT]) {

}

void execute_source(int arg) {

}

void execute_taxi() {

}

void handler(int signal) {

}