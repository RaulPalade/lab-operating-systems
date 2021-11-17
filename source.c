#include "source.h"
#include "general.h"

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
    /* END INIT */

    sem_sync_source(semaphore);

    log_message("Going into execution cycle", DB);
    while (1) {
        nanosleep(&message_interval, NULL);
        while (found != 1) {
            log_message("Generating message", -1);
            message.destination.x = rand() % SO_WIDTH;
            message.destination.y = rand() % SO_HEIGHT;
            if (message.destination.x >= 0 && message.destination.y < SO_WIDTH && message.destination.y >= 0 &&
                message.destination.y < SO_HEIGHT) {
                lock(mutex);
                *readers++;
                if (*readers == 1) {
                    sem_wait(message.destination, writers);
                }
                unlock(mutex);
                found = cell_is_free(map_ptr, message.destination);
                lock(mutex);
                *readers--;
                if (*readers == 0) {
                    sem_signal(message.destination, writers);
                }
                unlock(mutex);
            }
        }
        log_message("Sending message", SILENCE);
        if (0) {
            printf("\tmsg((%ld),(%d,%d))\n", message.type, message.destination.x, message.destination.y);
        }
        if (msgsnd(id_message_queue_source_taxi, &message, sizeof(map_point), IPC_NOWAIT) < 0) {
            log_message("Queue full, try again", SILENCE);
            nanosleep(&message_interval, NULL);
            found = 0;
            continue;
        }

        msg_master.requests++;
        found = 0;
    }
}

int sem_sync_source(int semaphore) {
    struct sembuf buf;
    buf.sem_num = 0;
    buf.sem_op = 0;
    buf.sem_flg = 0;
    if (semop(semaphore, &buf, 1) < 0) {
        return -1;
    } else {
        return 0;
    }
}

void user_request() {
    message msg;
    int found;
    if (sem_sync_source(semaphore)) {
        kill(getppid(), SIGUSR2);
    }
    found = 1;

    while (found != 0) {
        msg.destination.x = rand() % SO_WIDTH;
        msg.destination.y = rand() % SO_HEIGHT;
        if (msg.destination.x >= 0 && msg.destination.x < SO_WIDTH &&
            msg.destination.y >= 0 && msg.destination.y < SO_HEIGHT) {
            lock(mutex);
            *readers++;
            if (*readers == 1) {
                sem_wait(msg.destination, writers);
            }
            unlock(mutex);
            found = cell_is_free(map_ptr, msg.destination);
            lock(mutex);
            *readers--;
            if (*readers == 0) {
                sem_signal(msg.destination, writers);
            }
            unlock(mutex);
        }
        found = 0;
    }

    log_message(ANSI_COLOR_RED "Sending message:" ANSI_COLOR_RESET, RUNTIME);
    printf("\tmsg((%ld),(%d,%d))\n", msg.type, msg.destination.x, msg.destination.y);
    msgsnd(id_message_queue_source_taxi, &msg, sizeof(map_point), 0);
    msg_master.requests++;
    return;
}

void handler(int signal) {
    switch (signal) {
        case SIGALRM:
            raise(SIGINT);
            break;

        case SIGINT:
            log_message("Finishing up...", SILENCE);
            shmdt(map_ptr);
            shmdt(readers);
            msgsnd(id_message_queue_master_map_source, &msg_master, sizeof(int), 1);
            log_message("Received SIGUSR1", RUNTIME);
            exit(EXIT_SUCCESS);

        case SIGUSR1:
            log_message("Received SIUSR1", RUNTIME);
            break;

        case SIGTSTP:
            log_message("Received SIGTSTP", RUNTIME);
            user_request();
            break;
    }
}

void log_message(char *message, enum level l) {
    if (l <= DEBUG) {
        printf("[master-%d] %s\n", getpid(), message);
    }
}