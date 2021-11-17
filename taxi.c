#include "taxi.h"

int id_shared_memory_map;
int id_shared_memory_sources;
int id_shared_memory_readers;
int id_message_queue_source_taxi;
int id_message_queue_master_taxi;
int writers;
int mutex;
int semaphore;
int semaphore_source;
cell (*map_ptr)[][SO_HEIGHT];
map_point (*source_list_ptr)[];
int *readers;
map_point position;
int timensec_min;
int timensec_max;
int timeout;
int n_sources;
taxi_message data_message;
struct timeval timer;

int main(int argc, char **argv) {
    int received;
    key_t key;
    message message;
    const struct timespec nsecs[] = {0, 500000000L};
    struct sigaction sa;

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

    if ((key = ftok("./makefile", 'b')) < 0) {
        log_message("Error: ftok", RUNTIME);
        EXIT_ON_ERROR
    }

    if ((id_shared_memory_sources = shmget(key, 0, 0644)) < 0) {
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
    if ((key = ftok("./makefile", 'd')) < 0) {
        log_message("Error: ftok", RUNTIME);
        raise(SIGQUIT);
    }

    if ((id_message_queue_master_taxi = msgget(key, 0644)) < 0) {
        log_message("Error: msgget", RUNTIME);
        raise(SIGQUIT);
    } else {
        printf("id_message_queue_master_taxi = %d\n", id_message_queue_master_taxi);
    }

    if ((key = ftok("./makefile", 'q')) < 0) {
        log_message("Error: ftok", RUNTIME);
        raise(SIGQUIT);
    }

    if ((id_message_queue_source_taxi = msgget(key, 0644)) < 0) {
        log_message("Error: msgget", RUNTIME);
        EXIT_ON_ERROR
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

void increase_traffic_at_point(map_point p) {

}

void decrease_traffic_at_point(map_point p) {

}

void move_to_point(map_point destination) {

}

map_point get_near_source() {
    map_point mp;
    return mp;
}

void check_time_out() {

}

void print_report() {

}

void handler(int signal) {

}