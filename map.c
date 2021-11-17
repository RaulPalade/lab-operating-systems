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

    parse_configuration(&configuration);
    top_cells.requests = configuration.SO_TOP_CELLS;
    top_cells.type = 2;

    if (DEBUG) {
        log_message("Testing map", DB);
        for (col = 0; col < SO_WIDTH; col++) {
            for (row = 0; row < SO_HEIGHT; row++) {
                (*map_ptr)[col][row].state = FREE;
                (*map_ptr)[col][row].capacity = 100;
            }
        }
        log_message("OK", DB);
    }

    log_message("Generating Map...", DB);
    generate_map(map_ptr, &configuration);
    srand(time(NULL) + getpid());
    log_message("Init complete", DB);
    /* END INIT */

    log_message("Printing Map...", DB);
    print_map(map_ptr);
    log_message("Forking sources...", DB);

    for (i = 1; i < configuration.SO_SOURCES; i++) {
        if (DEBUG) {
            printf("\tSources n. %d created\n", i);
        }
        switch (fork()) {
            case -1:
                EXIT_ON_ERROR
            case 0:
                execute_source(i);
        }
    }

    log_message("Forking taxis...", DB);
    for (i = 1; i < configuration.SO_TAXI + 1; i++) {
        if (DEBUG) {
            printf("\tTaxi n. %d created\n", i);
        }
        switch (fork()) {
            case -1:
                EXIT_ON_ERROR
            case 0:
                execute_taxi();
        }
    }

    msgsnd(id_message_queue_master_map_source, &top_cells, sizeof(int), 0);
    unblock(semaphore);
    log_message("Starting Timer Now", DB);
    if (DEBUG) {
        printf("\tAlarm in %d seconds\n", configuration.SO_DURATION);
    }

    alarm(configuration.SO_DURATION);
    kill(getppid(), SIGUSR1);
    log_message("Waiting for children...", DB);
    while (executing) {
        if (dead_taxis > 0) {
            log_message("Relaunchin taxi...", DB);
            for (; dead_taxis > 0; dead_taxis--) {
                switch (fork()) {
                    case -1:
                        EXIT_ON_ERROR

                    case 0:
                        execute_taxi();
                        break;
                }
            }
        }
    }

    kill(0, SIGALRM);
    while (wait(NULL) > 0) {

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
    struct sembuf buf;
    buf.sem_num = 0;
    buf.sem_op = 1;
    buf.sem_flg = 0;
    if (semop(semaphore, &buf, 1) < 0) {
        EXIT_ON_ERROR
    }
}

void parse_configuration(simulation_configuration *configuration) {
    FILE *file;
    char s[16];
    char c;
    int n;
    int i = 0;
    char filename[] = "taxicab.conf";
    file = fopen(filename, "r");

    while (fscanf(file, "%s", s) == 1) {
        switch (s[0]) {
            case '#':
                do {
                    c = fgetc(file);
                } while (c != '\n');
                break;

            default:
                fscanf(file, "%d\n", &n);
                if (n < 0) {
                    log_message("Configuration not valid", RUNTIME);
                    kill(0, SIGINT);
                }

                if (strncmp(s, "SO_HOLES", 8) == 0) {
                    configuration->SO_HOLES = n;
                } else if (strncmp(s, "SO_TOP_CELLS", 12) == 0) {
                    configuration->SO_TOP_CELLS = n;
                } else if (strncmp(s, "SO_SOURCES", 10) == 0) {
                    configuration->SO_SOURCES = n;
                } else if (strncmp(s, "SO_CAP_MIN", 10) == 0) {
                    configuration->SO_CAP_MIN = n;
                } else if (strncmp(s, "SO_CAP_MAX", 10) == 0) {
                    configuration->SO_CAP_MAX = n;
                } else if (strncmp(s, "SO_TAXI", 7) == 0) {
                    configuration->SO_TAXI = n;
                } else if (strncmp(s, "SO_TIMENSEC_MIN", 15) == 0) {
                    configuration->SO_TIMENSEC_MIN = n;
                } else if (strncmp(s, "SO_TIMENSEC_MAX", 15) == 0) {
                    configuration->SO_TIMENSEC_MAX = n;
                } else if (strncmp(s, "SO_TIMEOUT", 10) == 0) {
                    configuration->SO_TIMEOUT = n;
                } else if (strncmp(s, "SO_DURATION", 11) == 0) {
                    configuration->SO_DURATION = n;
                }
                i++;
        }
    }

    if (i < 10) {
        log_message("Configuration not valid, less than 10 parameters", RUNTIME);
        kill(0, SIGINT);
    }

    fclose(file);
}

/**
 * Check for adjacent cells at matrix[x][y] marked as HOLE
 */
int check_no_adjacent_holes(cell (*matrix)[][SO_HEIGHT], int x, int y) {
    int found_adjcatent_holes = 0;
    int i;
    int j;
    time_t start_time;
    start_time = time(NULL);
    for (i = 0; i < 3; i++) {
        for (j = 0; j < 3; j++) {
            if (time(NULL) - start_time > 1) {
                log_message("You selected too many holes to fit the map:\n\t\tRetry with less.\nQuitting...", RUNTIME);
                kill(0, SIGINT);
            }
            if ((x + i - 1) >= 0 &&
                (x + i - 1) <= SO_WIDTH &&
                (y + j - 1) >= 0 &&
                (y + j - 1) <= SO_HEIGHT &&
                (*matrix)[x + i - 1][y + j - 1].state == HOLE) {
                found_adjcatent_holes = 1;
            }
        }
    }

    return found_adjcatent_holes;
}

void generate_map(cell (*matrix)[][SO_HEIGHT], simulation_configuration *configuration) {
    int x;
    int y;
    int r;
    int i;
    time_t start_time;
    if (SO_WIDTH <= 0 || SO_HEIGHT <= 0) {
        log_message("You must set appropriate SO_WIDTH and SO_HEIGHT:\n\t\tRetry\nQuitting...", RUNTIME);
        kill(0, SIGINT);
    }
    if (configuration->SO_HOLES + configuration->SO_SOURCES > SO_HEIGHT * SO_WIDTH) {
        log_message("You must set appropriate SO_WIDTH and SO_HEIGHT:\n\t\tRetry\nQuitting...", RUNTIME);
        kill(0, SIGINT);
    }

    start_time = time(NULL);

    for (x = 0; x < SO_WIDTH; x++) {
        if (time(NULL) - start_time > 4) {
            log_message("Could not generate the MAP:\n\t\tRetry with a smaller size.\nQuitting...", RUNTIME);
            kill(0, SIGINT);
        }
    }

    for (y = 0; y < SO_HEIGHT; y++) {
        (*matrix)[x][y].state = FREE;
        (*matrix)[x][y].traffic = 0;
        (*matrix)[x][y].visits = 0;
        r = rand();
        if (configuration->SO_CAP_MAX == configuration->SO_CAP_MIN) {
            (*matrix)[x][y].capacity = configuration->SO_CAP_MIN;
        } else {
            (*matrix)[x][y].capacity =
                    (r % (configuration->SO_CAP_MAX - configuration->SO_CAP_MIN)) + configuration->SO_CAP_MIN;
        }
    }

    /* To stop the user from using too many holes */
    start_time = time(NULL);
    for (i = configuration->SO_HOLES; i > 0; i--) {
        if (time(NULL) - start_time > 2) {
            log_message("You selected too many holes to fit the map:\n\t\tRetry with less.\nQuitting...", RUNTIME);
            kill(0, SIGINT);
        }
    }

    x = rand() % SO_WIDTH;
    y = rand() % SO_HEIGHT;

    if (check_no_adjacent_holes(matrix, x, y) == 0) {
        (*matrix)[x][y].state = HOLE;
    } else {
        i++;
    }

    /* To stop the user from using too many sources */
    start_time = time(NULL);
    for (i = 0; i < configuration->SO_SOURCES; i++) {
        log_message("HOLE", RUNTIME);
        if (time(NULL) - start_time > 1) {
            for (x = 0; x < SO_WIDTH; x++) {
                for (y = 0; y < SO_HEIGHT; y++) {
                    if ((*matrix)[x][y].state != HOLE) {
                        (*matrix)[x][y].state = SOURCE;
                    } else {
                        i--;
                    }
                    if (time(NULL) - start_time > 3) {
                        log_message("You selected too many sources to fit the map:\n\t\tRetry with less.\nQuitting...",
                                    RUNTIME);
                        kill(0, SIGINT);
                    }
                }
            }
        } else {
            x = rand() % SO_WIDTH;
            y = rand() % SO_HEIGHT;
            if ((*matrix)[x][y].state == FREE) {
                (*matrix)[x][y].state = SOURCE;
                (*source_list_ptr)[i].x = x;
                (*source_list_ptr)[i].y = y;
            } else {
                i--;
            }
        }
    }
}

/**
 * Print on stdout the map in a readable format:
 * [ ] for FREE Cells
 * [S] for SOURCE Cells
 * [#] for HOLE Cells
 */
void print_map(cell (*map)[][SO_HEIGHT]) {
    int x;
    int y;
    for (y = 0; y < SO_HEIGHT; y++) {
        for (x = 0; x < SO_WIDTH; x++) {
            switch ((*map)[x][y].state) {
                case FREE:
                    printf("[ ]");
                    break;

                case SOURCE:
                    printf("[S]");
                    break;

                case HOLE:
                    printf("[#]");
            }
        }
        printf("\n");
    }
    printf("\n");
}

void execute_source(int arg) {
    char arg_buffer[5];
    char *args[3];
    sprintf(arg_buffer, "%d", arg);
    args[0] = "source";
    args[1] = arg_buffer;
    args[2] = NULL;
    execv("source", args);
}

void execute_taxi() {
    map_point p;
    int x;
    int y;
    int found = 0;
    int start_time;
    char arg_x[5];
    char arg_y[5];
    char arg_min[5];
    char arg_max[5];
    char arg_time[5];
    char arg_sources[5];
    char *args[8];
    args[0] = "taxi";
    srand(time(NULL) ^ (getpid() << 16));
    start_time = time(NULL);
    while (found != 1) {
        if (time(NULL) - start_time > 1) {
            for (x = 0; x < SO_WIDTH; x++) {
                for (y = 0; y < SO_HEIGHT; y++) {
                    if ((*map_ptr)[x][y].state != HOLE &&
                        (*map_ptr)[p.x][p.y].traffic < (*map_ptr)[p.x][p.y].capacity) {
                        found = 1;
                        continue;
                    } else if (time(NULL) - start_time > 3) {
                        log_message("Could not fit taxi", DB);
                        exit(EXIT_SUCCESS);
                    }
                }
            }
        } else {
            x = rand() % SO_WIDTH;
            y = rand() % SO_HEIGHT;
            if (x >= 0 && x < SO_WIDTH && y >= 0 && y < SO_HEIGHT) {
                p.x = x;
                p.y = y;
                if ((*map_ptr)[p.x][p.y].state != HOLE &&
                    ((*map_ptr)[p.x][p.y].traffic < (*map_ptr)[p.x][p.y].capacity)) {
                    found = 1;
                }
            }
        }
    }

    sprintf(arg_x, "%d", x);
    args[1] = arg_x;
    sprintf(arg_y, "%d", y);
    args[2] = arg_y;
    sprintf(arg_min, "%d", configuration.SO_TIMENSEC_MIN);
    args[3] = arg_min;
    sprintf(arg_max, "%d", configuration.SO_TIMENSEC_MAX);
    args[4] = arg_max;
    sprintf(arg_time, "%d", configuration.SO_TIMEOUT);
    args[5] = arg_time;
    sprintf(arg_sources, "%d", configuration.SO_SOURCES);
    args[6] = arg_sources;
    args[7] = NULL;

    execv("taxi", args);
    exit(EXIT_SUCCESS);
}

void handler(int signal) {
    switch (signal) {
        case SIGINT:
            if (DEBUG) {
                printf("\n============== CLOSING ==============\n");
            }
            executing = 0;
            msgsnd(id_message_queue_master_map_source, &top_cells, sizeof(int), IPC_NOWAIT);
            while (wait(NULL) > 0) {

            }
            shmdt(map_ptr);
            shmdt(source_list_ptr);
            shmdt(readers);
            if (shmctl(id_shared_memory_sources, IPC_RMID, NULL)) {
                printf("Error in shctl sources\n");
            }
            if (shmctl(id_shared_memory_readers, IPC_RMID, NULL)) {
                printf("Error in shctl readers\n");
            }

            if (msgctl(id_message_queue_source_taxi, IPC_RMID, NULL)) {
                printf("Error in msgctl source-taxi\n");
            }
            if (semctl(writers, 0, IPC_RMID)) {
                printf("Error in semctl writers\n");
            }
            if (semctl(semaphore, 0, IPC_RMID)) {
                printf("Error in semctl semaphore\n");
            }
            if (semctl(mutex, 0, IPC_RMID)) {
                printf("Error in semctl mutex\n");
            }
            log_message("Graceful exit successful", DB);
            kill(getppid(), SIGUSR2);
            exit(EXIT_SUCCESS);

        case SIGALRM:
            executing = 0;
            break;

        case SIGQUIT:
            shmdt(map_ptr);
            shmdt(source_list_ptr);
            shmdt(readers);
            if (shmctl(id_shared_memory_sources, IPC_RMID, NULL)) {
                printf("Error in shctl sources\n");
            }
            if (shmctl(id_shared_memory_readers, IPC_RMID, NULL)) {
                printf("Error in shctl readers\n");
            }

            if (msgctl(id_message_queue_source_taxi, IPC_RMID, NULL)) {
                printf("Error in msgctl source-taxi\n");
            }
            if (semctl(writers, 0, IPC_RMID)) {
                printf("Error in semctl writers\n");
            }
            if (semctl(semaphore, 0, IPC_RMID)) {
                printf("Error in semctl semaphore\n");
            }
            if (semctl(mutex, 0, IPC_RMID)) {
                printf("Error in semctl mutex\n");
            }
            log_message("Graceful exit successful", DB);
            kill(getppid(), SIGUSR2);
            exit(EXIT_SUCCESS);

        case SIGUSR1:
            log_message("Received SIGUSR1", DB);
            dead_taxis++;
            break;

        case SIGUSR2:
            log_message("Received SIGUSR2", DB);
            break;

        case SIGTSTP:
            break;
    }
}

void log_message(char *message, enum level l) {
    if (l <= DEBUG) {
        printf("[master-%d] %s\n", getpid(), message);
    }
}