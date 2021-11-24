#ifndef __GENERAL_H_
#define __GENERAL_H_

#include <errno.h>
#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#ifndef DEBUG
#define DEBUG 0
#endif

#define SO_WIDTH    53
#define SO_HEIGHT   21
#define EXIT_ON_ERROR                                                                                           \
    if (errno) {                                                                                                \
        fprintf(stderr, "%d: pid %ld; errno: %d (%s)\n", __LINE__, (long)getpid(), errno, strerror(errno));     \
        raise(SIGINT);                                                                                          \
    }

#define ANSI_COLOR_RED "\x1b[31m"
#define ANSI_COLOR_RESET "\x1b[0m"

enum type {
    FREE, SOURCE, HOLE
};

enum level {
    RUNTIME, DB, SILENCE
};

typedef struct {
    enum type state;
    int capacity;
    int traffic;
    int visits;
} cell;

typedef struct {
    int SO_TAXI;
    int SO_SOURCES;
    int SO_HOLES;
    int SO_TOP_CELLS;
    int SO_CAP_MIN;
    int SO_CAP_MAX;
    int SO_TIMENSEC_MIN;
    int SO_TIMENSEC_MAX;
    int SO_TIMEOUT;
    int SO_DURATION;
} simulation_configuration;

typedef struct {
    int x;
    int y;
} map_point;

typedef struct {
    long type;
    map_point destination;
} message;

typedef struct {
    long type;
    int requests;
} master_message;

void log_message(char *, enum level);

int cell_is_free(cell (*map)[][SO_HEIGHT], map_point p);

void sem_wait(map_point, int);

void sem_signal(map_point, int);

void sem_sync(int);

void lock(int);

void unlock(int);

union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
    struct seminfo *__buf;
};

#endif