#include "general.h"

int cell_is_free(cell (*map)[][SO_HEIGHT], map_point p) {
    return (*map)[p.x][p.y].state != HOLE ? 1 : 0;
}

void sem_wait(map_point p, int semaphore) {
    struct sembuf buf;
    buf.sem_num = p.y * SO_WIDTH + p.x;
    buf.sem_op = -1;
    buf.sem_flg = SEM_UNDO;
    if (semop(semaphore, &buf, 1) < 0) {
        log_message("Failed sem_wait", DB);
        kill(getppid(), SIGUSR1);
        raise(SIGINT);
    }
}

void sem_signal(map_point p, int semaphore) {
    struct sembuf buf;
    buf.sem_num = p.y * SO_WIDTH + p.x;
    buf.sem_op = 1;
    buf.sem_flg = SEM_UNDO;
    if (semop(semaphore, &buf, 1) < 0) {
        log_message("Failed sem_signal", DB);
        kill(getppid(), SIGUSR1);
        raise(SIGINT);
    }
}

void sem_sync(int semaphore) {
    struct sembuf buf;
    buf.sem_num = 0;
    buf.sem_op = 0;
    buf.sem_flg = 0;
    if (semop(semaphore, &buf, 1) < 0) {
        kill(getppid(), SIGUSR1);
        raise(SIGINT);
    }
}

void lock(int semaphore) {
    struct sembuf buf;
    buf.sem_num = 0;
    buf.sem_op = -1;
    buf.sem_flg = SEM_UNDO;
    if (semop(semaphore, &buf, 1) < 0) {
        log_message("Failed lock", DB);
        kill(getppid(), SIGUSR1);
        raise(SIGINT);
    }
}

void unlock(int semaphore) {
    struct sembuf buf;
    buf.sem_num = 0;
    buf.sem_op = 1;
    buf.sem_flg = SEM_UNDO;
    if (semop(semaphore, &buf, 1) < 0) {
        log_message("Failed unlock", DB);
        kill(getppid(), SIGUSR1);
        raise(SIGINT);
    }
}