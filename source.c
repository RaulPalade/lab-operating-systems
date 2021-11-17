#include "source.h"

master_message msg_master;
cell (*map_ptr)[][SO_HEIGHT];
int id_shared_memory_map;
int id_shared_memory_readers;
int id_message_queue_master_map_source;
int id_message_queue_source_taxi;
int *readers;
int found = 0;
int semaphore;
int writers;
int mutex;

int main(int argc, char **argv) {
    key_t key;
    message message;
    struct timespec message_interval;
    struct sigaction sa;
    struct msqid_ds ds;

    log_message("Init", DB);
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handler;

    sigaction(SIGINT, &sa, 0);
    sigaction(SIGALRM, &sa, 0);
    sigaction(SIGUSR1, &sa, 0);
    sigaction(SIGUSR2, &sa, 0);
    sigaction(SIGTSTP, &sa, 0);

    /* SHARED MEMORY MANAGEMENT */
    if ((key = ftok("./makefile", 'm')) < 0) {
        log_message("Error: ftok", RUNTIME);
        EXIT_ON_ERROR
    }

    if ((id_shared_memory_map = shmget(key, 0, 0644)) < 0) {
        log_message("Error: shmget", RUNTIME);
        EXIT_ON_ERROR
    } else {
        printf("id_shared_memory_map = %d\n", id_shared_memory_map);
    }

    if ((void *) (map_ptr = shmat(id_shared_memory_map, NULL, 0)) < (void *) 0) {
        log_message("Error: shmat", RUNTIME);
        EXIT_ON_ERROR
    }

    if ((key = ftok("./makefile", 'z')) < 0) {
        log_message("Error: ftok", RUNTIME);
        EXIT_ON_ERROR
    }

    if ((id_shared_memory_readers = shmget(key, 0, 0666)) < 0) {
        log_message("Error: shmget", RUNTIME);
        EXIT_ON_ERROR
    } else {
        printf("id_shared_memory_readers=%d\n", id_shared_memory_readers);
    }

    if ((void *) (readers = shmat(id_shared_memory_readers, NULL, 0)) < (void *) 0) {
        log_message("Error: shmat", RUNTIME);
        EXIT_ON_ERROR
    }

    /* MESSAGE QUEUE MANAGEMENT */
    if ((key = ftok("./makefile", 's')) < 0) {
        log_message("Error: ftok", RUNTIME);
        EXIT_ON_ERROR
    }

    if ((id_message_queue_master_map_source = msgget(key, 0644)) < 0) {
        log_message("Error: msgget", RUNTIME);
        EXIT_ON_ERROR
    } else {
        printf("id_message_queue_master_map_source=%d\n", id_message_queue_master_map_source);
    }

    if ((key = ftok("./makefile", 'q')) < 0) {
        log_message("Error: ftok", RUNTIME);
        EXIT_ON_ERROR
    }

    if ((id_message_queue_source_taxi = msgget(key, 0644)) < 0) {
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

    if ((writers = semget(key, 0, 0666)) < 0) {
        log_message("Error: semget", RUNTIME);
        EXIT_ON_ERROR
    } else {
        printf("writers=%d\n", writers);
    }

    if ((key = ftok("./makefile", 'm')) < 0) {
        log_message("Error: ftok", RUNTIME);
        EXIT_ON_ERROR
    }

    if ((mutex = semget(key, 0, 0666)) < 0) {
        log_message("Error: semget", RUNTIME);
        EXIT_ON_ERROR
    } else {
        printf("mutex=%d\n", mutex);
    }

    if ((key = ftok("./makefile", 'y')) < 0) {
        log_message("Error: ftok", RUNTIME);
        EXIT_ON_ERROR
    }

    if ((semaphore = semget(key, 0, 0666)) < 0) {
        log_message("Error: semget", RUNTIME);
        EXIT_ON_ERROR
    } else {
        printf("semaphore=%d\n", semaphore);
    }
}

int sem_sync_source(int semaphore) {
    return 1;
}

void user_request() {

}

void handler(int signal) {

}