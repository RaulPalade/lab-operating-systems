#include "master.h"
#include "general.h"

cell (*map_ptr)[][SO_HEIGHT];
volatile int executing = 1;
simulation_data sim_data;
int id_shared_memory_map;
int id_message_queue_master_map_source;
int id_message_queue_master_taxi;

int main() {
    char *args[2];
    int t;
    key_t key;
    taxi_message taxi_message;
    source_message source_message;
    struct msqid_ds message_queue_ds;
    struct sigaction sa;

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
        raise(SIGQUIT);
    }

    if ((id_shared_memory_map = shmget(key, SO_WIDTH * SO_HEIGHT * sizeof(cell), IPC_CREAT | 0666)) < 0) {
        log_message("Error: shmget", RUNTIME);
        raise(SIGQUIT);
    } else {
        printf("id_shared_memory_map = %d\n", id_shared_memory_map);
    }

    if ((void *) (map_ptr = shmat(id_shared_memory_map, NULL, 0)) < (void *) 0) {
        log_message("Error: shmat", RUNTIME);
        raise(SIGQUIT);
    }

    /* MESSAGE QUEUE MANAGEMENT */
    if ((key = ftok("./makefile", 's')) < 0) {
        log_message("Error: ftok", RUNTIME);
        raise(SIGQUIT);
    }

    if ((id_message_queue_master_map_source = msgget(key, IPC_CREAT | 0644)) < 0) {
        log_message("Error: msgget", RUNTIME);
        raise(SIGQUIT);
    } else {
        printf("id_message_queue_master_map_source = %d\n", id_message_queue_master_map_source);
    }

    if ((key = ftok("./makefile", 'd')) < 0) {
        log_message("Error: ftok", RUNTIME);
        raise(SIGQUIT);
    }

    if ((id_message_queue_master_taxi = msgget(key, IPC_CREAT | 0644)) < 0) {
        log_message("Error: msgget", RUNTIME);
        raise(SIGQUIT);
    } else {
        printf("id_message_queue_master_taxi = %d\n", id_message_queue_master_taxi);
    }

    log_message("=================\nLaunching Generator\n=================", DB);

    switch (fork()) {
        case -1:
            log_message("Error: fork", RUNTIME);
            raise(SIGQUIT);
            break;

        case 0:
            args[0] = "map";
            args[1] = NULL;
            execv("map", args);
            break;
    }


    msgrcv(id_message_queue_master_map_source, &source_message, sizeof(int), 2, 0);
    sim_data.top_cells = source_message.requests;
    sim_data.winner_cells = malloc(sizeof(map_point) * sim_data.top_cells);
    t = time(NULL);

    while (executing) {
        if ((time(NULL) - t) >= 1) {
            print_map(map_ptr);
            t = time(NULL);
        }
    }

    while (wait(NULL) > 0) {

    }

    sleep(1);

    msgctl(id_message_queue_master_map_source, IPC_STAT, &message_queue_ds);
    while (message_queue_ds.msg_qnum > 0) {
        if (msgrcv(id_message_queue_master_map_source, &source_message, sizeof(int), 0, IPC_NOWAIT) == -1) {
            perror("msgrcv");
            raise(SIGQUIT);
        }

        sim_data.requests += source_message.requests;
        msgctl(id_message_queue_master_map_source, IPC_STAT, &message_queue_ds);
    }

    msgctl(id_message_queue_master_taxi, IPC_STAT, &message_queue_ds);
    while (message_queue_ds.msg_qnum > 0) {
        if (msgrcv(id_message_queue_master_map_source, &taxi_message, sizeof(taxi_data), 0, IPC_NOWAIT) == -1) {
            perror("msgrcv");
            raise(SIGQUIT);
        }

        update_cells_data(taxi_message.type, &taxi_message.taxi_data);
        msgctl(id_message_queue_master_map_source, IPC_STAT, &message_queue_ds);
    }

    sim_data.not_served_travels = sim_data.requests - sim_data.max_travels;
    gathering_cells_data(map_ptr, sim_data.top_cells);
    print_map_report(map_ptr);


    sleep(10);

    printf("===================\nReturning to master\n===================\n");
    shmctl(id_shared_memory_map, IPC_RMID, NULL);
    msgctl(id_message_queue_master_map_source, IPC_RMID, NULL);
    msgctl(id_message_queue_master_taxi, IPC_RMID, NULL);
    free(sim_data.winner_cells);
    log_message("Simulation ending", DB);

    exit(EXIT_SUCCESS);
}

void gathering_cells_data(cell (*map)[][SO_HEIGHT], int l) {
    int row;
    int col;
    int n;
    int i;
    int tmp;
    int usage[SO_WIDTH * SO_HEIGHT];

    for (n = 0; n < SO_WIDTH * SO_HEIGHT; n++) {
        usage[n] = 0;
    }

    for (col = 0; col < SO_HEIGHT; col++) {
        for (row = 0; row < SO_WIDTH; row++) {
            if ((*map)[row][col].state == FREE) {
                for (n = 0; n < l; n++) {
                    if ((*map)[row][col].visits > usage[n]) {
                        tmp = n;
                        for (i = n; i < l; i++) {
                            if (usage[i] < usage[tmp]) {
                                tmp = i;
                            }
                        }
                        usage[tmp] = (*map)[row][col].visits;
                        (*sim_data.winner_cells)[tmp].x = row;
                        (*sim_data.winner_cells)[tmp].y = col;
                        break;
                    }
                }
            }
        }
    }
}

void update_cells_data(long pid, taxi_data *data) {
    sim_data.total_travels += data->clients;
    sim_data.success_travels += data->success_travel;

    if (sim_data.max_travels < data->clients) {
        sim_data.max_travels = data->clients;
        sim_data.travels_winner = pid;
    }

    if (sim_data.max_time.tv_sec <= data->max_travel_time.tv_sec) {
        if (sim_data.max_time.tv_usec < data->max_travel_time.tv_usec) {
            sim_data.max_time.tv_sec = data->max_travel_time.tv_sec;
            sim_data.max_time.tv_usec = data->max_travel_time.tv_usec;
            sim_data.time_winner = pid;
        }
    }

    if (sim_data.max_distance < data->distance) {
        sim_data.max_distance = data->distance;
        sim_data.distance_winner = pid;
    }
}

void print_map(cell (*map)[][SO_HEIGHT]) {
    int row;
    int col;
    for (col = 0; col < SO_HEIGHT; col++) {
        for (row = 0; row < SO_WIDTH; row++) {
            switch ((*map)[row][col].state) {
                case FREE:
                    printf("[%d]", (*map)[row][col].traffic);
                    break;

                case SOURCE:
                    printf("[S]");
                    break;

                case HOLE:
                    printf("[#]");
                    break;
            }
        }
        printf("\n");
    }
    printf("\n");
}

void print_map_report(cell (*map)[][SO_HEIGHT]) {
    int row;
    int col;
    int n;
    int db;

    printf("======== SIMULATION SUCCESS  ========\n");
    printf("Statistics:\n");
    printf("Travels:\n");
    printf("\t  \tSuccessful \tNot Served \tAborts\n");
    printf("\t  \t%d    \t%d    \t%d\n", sim_data.success_travels, sim_data.not_served_travels,
           (sim_data.total_travels - sim_data.success_travels));

    printf("\tTaxi:\n");
    printf("\t\tMost Distance   \tLongest Travels   \tMost Travels\n");
    printf("\tpid:\t%ld         \t%ld               \t%ld\n", sim_data.distance_winner, sim_data.time_winner,
           sim_data.travels_winner);
    printf("\t  \t%d        \t%ld ms    \t%d\n", sim_data.max_distance,
           (sim_data.max_time.tv_sec * 1000 + sim_data.max_time.tv_usec / 1000), sim_data.max_travels);

    printf("Top Cells:\n");
    printf("\t\tvisits \tx \ty\n");
    for (n = 0; n < sim_data.top_cells; n++) {
        printf("\t\t%d \t%d \t%d\n", (*map)[(*sim_data.winner_cells)[n].x][(*sim_data.winner_cells)[n].y].visits,
               (*sim_data.winner_cells)[n].x, (*sim_data.winner_cells)[n].y);
    }

    printf("\n");

    for (col = 0; col < SO_HEIGHT; col++) {
        for (row = 0; row < SO_WIDTH; row++) {
            switch ((*map)[row][col].state) {
                case FREE:
                    db = 0;
                    for (n = 0; n < sim_data.top_cells; n++) {
                        if ((*sim_data.winner_cells)[n].x == row && (*sim_data.winner_cells)[n].y == col
                            && (*map)[(*sim_data.winner_cells)[n].x][(*sim_data.winner_cells)[n].y].visits > 0) {
                            db = 1;
                            printf(ANSI_COLOR_RED "[ ]" ANSI_COLOR_RESET);
                        }
                    }
                    if (db == 0) {
                        printf("[ ]");
                    }
                    break;

                case SOURCE:
                    printf("[S]");
                    break;

                case HOLE:
                    printf("[#]");
                    break;
            }
        }
        printf("\n");
    }
}

void handler(int signal) {
    switch (signal) {
        case SIGINT:
            executing = 0;
            break;

        case SIGALRM:
            executing = 0;
            break;

        case SIGQUIT:
            log_message("SIGQUIT Signal received", DB);
            shmctl(id_shared_memory_map, IPC_RMID, NULL);
            msgctl(id_message_queue_master_map_source, IPC_RMID, NULL);
            msgctl(id_message_queue_master_taxi, IPC_RMID, NULL);
            free(sim_data.winner_cells);
            exit(EXIT_SUCCESS);

        case SIGUSR1:
            break;

        case SIGUSR2:
            executing = 0;
            break;

        case SIGTSTP:
            executing = 0;
            break;

        default:
            break;
    }
}

void log_message(char *message, enum level l) {
    if (l <= DEBUG) {
        printf("[master-%d] %s\n", getpid(), message);
    }
}