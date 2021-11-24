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

    log_message("Semaphore Init OK", DB);
    sem_sync(semaphore);
    log_message("Sync OK", DB);

    sscanf(argv[1], "%d", &position.x);
    sscanf(argv[2], "%d", &position.y);
    sscanf(argv[3], "%d", &timensec_min);
    sscanf(argv[4], "%d", &timensec_max);
    sscanf(argv[5], "%d", &timeout);
    sscanf(argv[6], "%d", &n_sources);
    srand(time(NULL) + getpid());
    data_message.type = getpid();
    data_message.taxi_data.distance = 0;
    data_message.taxi_data.longest_travel = 0;
    data_message.taxi_data.max_travel_time.tv_sec = 0;
    data_message.taxi_data.max_travel_time.tv_usec = 0;
    data_message.taxi_data.clients = 0;
    data_message.taxi_data.success_travel = 0;
    data_message.taxi_data.abort = 0;

    log_message("Init Finished", DB);
    /* END INIT */

    increase_traffic_at_point(position);
    while (1) {
        gettimeofday(&timer, NULL);
        log_message("Going to nearest source", DB);
        move_to_point(get_near_source(&id_shared_memory_sources));
        received = 0;

        while (!received) {
            log_message("Listening for call on:", DB);
            if (DEBUG) {
                printf("Source[%d](%d,%d)\n", id_shared_memory_sources, position.x, position.y);
            }
            if (msgrcv(id_message_queue_source_taxi, &message, sizeof(map_point), id_shared_memory_sources, 0) < 0) {
                check_time_out();
                log_message("Timeout passed, waiting for message", SILENCE);
            } else {
                received = 1;
            }
        }
        data_message.taxi_data.clients++;
        log_message("Going to destination", DB);
        move_to_point(message.destination);
        data_message.taxi_data.success_travel++;
    }
}

void increase_traffic_at_point(map_point p) {
    if (!(p.x >= 0 && p.x < SO_WIDTH && p.y >= 0 && p.y < SO_HEIGHT)) {
        EXIT_ON_ERROR
    }
    sem_wait(p, writers);
    (*map_ptr)[p.x][p.y].traffic++;
    sem_signal(p, writers);
}

void decrease_traffic_at_point(map_point p) {
    if (!(p.x >= 0 && p.x < SO_WIDTH && p.y >= 0 && p.y < SO_HEIGHT)) {
        EXIT_ON_ERROR
    }
    sem_wait(p, writers);
    (*map_ptr)[p.x][p.y].traffic--;
    sem_signal(p, writers);
}

int can_transit(map_point p) {
    int can_transit;
    if (!(p.x >= 0 && p.x < SO_WIDTH && p.y >= 0 && p.y < SO_HEIGHT)) {
        EXIT_ON_ERROR
    }
    lock(mutex);
    *readers++;
    if (*readers == 1) {
        sem_wait(p, writers);
    }
    unlock(mutex);
    can_transit = cell_is_free(map_ptr, p) && (*map_ptr)[p.x][p.y].traffic < (*map_ptr)[p.x][p.y].capacity;
    lock(mutex);
    *readers--;
    if (*readers == 0) {
        sem_signal(p, writers);
    }
    unlock(mutex);

    return can_transit;
}

map_point get_near_source(int *id_shared_memory_sources) {
    map_point p;
    int n;
    int tmp;
    int d = INT_MAX;
    for (n = 0; n < n_sources; n++) {
        tmp = abs(position.x - (*source_list_ptr)[n].x) + abs(position.y - (*source_list_ptr)[n].y);
        if (d > tmp) {
            d = tmp;
            p = (*source_list_ptr)[n];
            *id_shared_memory_sources = n + 1;
        }
    }

    return p;
}

void check_time_out() {
    struct timeval elapsed;
    int s;
    int u;
    int n;
    log_message("Timeout check", SILENCE);
    gettimeofday(&elapsed, NULL);
    s = elapsed.tv_sec - timer.tv_sec;
    u = elapsed.tv_usec - timer.tv_usec;
    n = s * 1000000000 + (u * 1000);
    if (n >= timeout) {
        log_message("Timeout", DB);
        (*map_ptr)[position.x][position.y].traffic--;
        data_message.taxi_data.abort++;
        log_message("Finishing up", SILENCE);
        shmdt(map_ptr);
        shmdt(source_list_ptr);
        shmdt(readers);
        msgsnd(id_message_queue_master_taxi, &data_message, sizeof(taxi_data), 0);
        log_message("Graceful exit successful", DB);
        print_report();
        kill(getppid(), SIGUSR1);
        exit(EXIT_SUCCESS);
    } else {
        return;
    }
}

void move_to_point(map_point destination) {
    int dir_x;
    int dir_y;
    int i;
    int found;
    int old_distance;
    long t1;
    long t2;
    struct timespec transit;
    struct timeval start;
    struct timeval end;
    map_point temp;
    map_point old_pos;
    log_message("Moving to destination:", SILENCE);

    old_distance = data_message.taxi_data.distance;
    gettimeofday(&start, NULL);
    gettimeofday(&timer, NULL);
    old_pos.x = position.x;
    old_pos.y = position.y;

    while (position.x != destination.x || position.y != destination.y) {
        check_time_out();
        t2 = 0;

        if (0)
            printf("[taxi-%d] pos: (%d,%d)\n", getpid(), position.x, position.y);
        if (0)
            printf("[taxi-%d]--->(%d,%d)\n", getpid(), destination.x, destination.y);
        dir_x = destination.x - position.x;
        dir_y = destination.y - position.y;
        temp.x = position.x;
        temp.y = position.y;
        found = 0;
        if (0)
            printf("[taxi-%d] dir_x: %d dir_y %d\n", getpid(), dir_x, dir_y);
        if (abs(dir_x + dir_y) == 1) {
            lock(mutex);
            increase_traffic_at_point(destination);
            decrease_traffic_at_point(position);
            unlock(mutex);
            position.x = destination.x;
            position.y = destination.y;
            if (0)
                printf("[taxi-%d] arrived at: (%d,%d)\n", getpid(), destination.x, destination.y);
            break;
        }
        if (dir_x == 0 && dir_y > 0 && (old_pos.y != position.y + 1) &&
            (*map_ptr)[temp.x][temp.y + 1].state == FREE) {
            temp.y++;
            while (!found) {
                check_time_out();
                if (can_transit(temp)) {
                    lock(mutex);
                    increase_traffic_at_point(temp);
                    decrease_traffic_at_point(position);
                    unlock(mutex);
                    old_pos.x = position.x;
                    old_pos.y = position.y;
                    position.x = temp.x;
                    position.y = temp.y;
                    found = 1;
                }
            }
        } else if (dir_x == 0 && dir_y < 0 && (old_pos.y != position.y - 1) &&
                   (*map_ptr)[temp.x][temp.y - 1].state == FREE) {
            temp.y--;
            while (!found) {
                check_time_out();
                if (can_transit(temp)) {
                    lock(mutex);
                    increase_traffic_at_point(temp);
                    decrease_traffic_at_point(position);
                    unlock(mutex);
                    old_pos.x = position.x;
                    old_pos.y = position.y;
                    position.x = temp.x;
                    position.y = temp.y;
                    found = 1;
                }
            }
        } else if (dir_y == 0 && dir_x > 0 && (old_pos.x != position.x + 1) &&
                   (*map_ptr)[temp.x + 1][temp.y].state == FREE) {
            temp.x++;
            while (!found) {
                check_time_out();
                if (can_transit(temp)) {
                    lock(mutex);
                    increase_traffic_at_point(temp);
                    decrease_traffic_at_point(position);
                    unlock(mutex);
                    old_pos.x = position.x;
                    old_pos.y = position.y;
                    position.x = temp.x;
                    position.y = temp.y;
                    found = 1;
                }
            }
        } else if (dir_y == 0 && dir_x < 0 && (old_pos.x != position.x - 1) &&
                   (*map_ptr)[temp.x - 1][temp.y].state == FREE) {
            temp.x--;
            while (!found) {
                check_time_out();
                if (can_transit(temp)) {
                    lock(mutex);
                    increase_traffic_at_point(temp);
                    decrease_traffic_at_point(position);
                    unlock(mutex);
                    old_pos.x = position.x;
                    old_pos.y = position.y;
                    position.x = temp.x;
                    position.y = temp.y;
                    found = 1;
                }
            }
        } else if (dir_x >= 0 && dir_y >= 0) {
            for (i = 0; i < 5 && !found; i++) {
                switch (i) {
                    case 0:
                        temp.x++;
                        break;
                    case 1:
                        temp.y++;
                        break;
                    case 2:
                        temp.x--;
                        break;
                    case 3:
                        temp.y--;
                        break;
                    case 4:
                        i = -1;
                        continue;
                }
                check_time_out();
                if (old_pos.x == temp.x && old_pos.y == temp.y) {
                    temp.x = position.x;
                    temp.y = position.y;
                    continue;
                } else {
                    if (!can_transit(temp))
                        continue;
                    lock(mutex);
                    increase_traffic_at_point(temp);
                    decrease_traffic_at_point(position);
                    unlock(mutex);
                    old_pos.x = position.x;
                    old_pos.y = position.y;
                    position.x = temp.x;
                    position.y = temp.y;
                    found = 1;
                }
            }
        } else if (dir_x >= 0 && dir_y <= 0) {
            for (i = 0; i < 5 && !found; i++) {
                switch (i) {
                    case 0:
                        temp.x++;
                        break;
                    case 1:
                        temp.y--;
                        break;
                    case 2:
                        temp.x--;
                        break;
                    case 3:
                        temp.y++;
                        break;
                    case 4:
                        i = -1;
                        continue;
                }
                check_time_out();
                if (old_pos.x == temp.x && old_pos.y == temp.y) {
                    temp.x = position.x;
                    temp.y = position.y;
                    continue;
                } else {
                    if (!can_transit(temp))
                        continue;
                    lock(mutex);
                    increase_traffic_at_point(temp);
                    decrease_traffic_at_point(position);
                    unlock(mutex);
                    old_pos.x = position.x;
                    old_pos.y = position.y;
                    position.x = temp.x;
                    position.y = temp.y;
                    found = 1;
                }
            }
        } else if (dir_x <= 0 && dir_y >= 0) {
            for (i = 0; i < 5 && !found; i++) {
                switch (i) {
                    case 0:
                        temp.y++;
                        break;
                    case 1:
                        temp.y--;
                        break;
                    case 2:
                        temp.x--;
                        break;
                    case 3:
                        temp.x++;
                        break;
                    case 4:
                        i = -1;
                        continue;
                }
                check_time_out();
                if (old_pos.x == temp.x && old_pos.y == temp.y) {
                    temp.x = position.x;
                    temp.y = position.y;
                    continue;
                } else {
                    if (!can_transit(temp))
                        continue;
                    lock(mutex);
                    increase_traffic_at_point(temp);
                    decrease_traffic_at_point(position);
                    unlock(mutex);
                    old_pos.x = position.x;
                    old_pos.y = position.y;
                    position.x = temp.x;
                    position.y = temp.y;
                    found = 1;
                }
            }
        } else if (dir_x <= 0 && dir_y <= 0) {
            for (i = 0; i < 5 && !found; i++) {
                switch (i) {
                    case 0:
                        temp.x--;
                        break;
                    case 1:
                        temp.y--;
                        break;
                    case 2:
                        temp.x++;
                        break;
                    case 3:
                        temp.y++;
                        break;
                    case 4:
                        i = -1;
                        continue;
                }
                check_time_out();
                if (old_pos.x == temp.x && old_pos.y == temp.y) {
                    temp.x = position.x;
                    temp.y = position.y;
                    continue;
                } else {
                    if (!can_transit(temp))
                        continue;
                    lock(mutex);
                    increase_traffic_at_point(temp);
                    decrease_traffic_at_point(position);
                    unlock(mutex);
                    old_pos.x = position.x;
                    old_pos.y = position.y;
                    position.x = temp.x;
                    position.y = temp.y;
                    found = 1;
                }
            }
        } /*END-else-ifs*/

        t1 = (long) (rand() % (timensec_max - timensec_min)) + timensec_min;
        if (t1 > 999999999L) {
            t2 = floor(t1 / 1000000000L);
            t1 = t1 - 1000000000L * t2;
        }
        transit.tv_sec = t2;
        transit.tv_nsec = t1;
        nanosleep(&transit, NULL);
        (*map_ptr)[position.x][position.y].visits++;
        data_message.taxi_data.distance++;
        gettimeofday(&timer, NULL);
    } /*END-while*/
    gettimeofday(&end, NULL);
    data_message.taxi_data.max_travel_time.tv_sec = (end.tv_sec - start.tv_sec);
    data_message.taxi_data.max_travel_time.tv_usec = labs(end.tv_usec - start.tv_usec);
    if ((data_message.taxi_data.distance - old_distance) >
        data_message.taxi_data.longest_travel) {
        data_message.taxi_data.longest_travel = data_message.taxi_data.distance - old_distance;
    }
    log_message("Arrived in", DB);
    if (DEBUG)
        printf("[taxi-%d]--->(%d,%d)\n", getpid(), destination.x, destination.y);
    gettimeofday(&timer, NULL);
}

void print_report() {
    if (DEBUG) {
        printf(ANSI_COLOR_RED "\ntaxiN°: %ld, distance: %i, Max distance: %i, Max time in travels: %ld ms, \n"
               "clients: %i, success travels: %i, abort travels: %i\n\n" ANSI_COLOR_RESET,
               data_message.type, data_message.taxi_data.distance, data_message.taxi_data.longest_travel,
               ((data_message.taxi_data.max_travel_time.tv_sec * 1000) +
                (data_message.taxi_data.max_travel_time.tv_usec / 1000)),
               data_message.taxi_data.clients, data_message.taxi_data.success_travel, data_message.taxi_data.abort);
    }
}

void handler(int signal) {
    switch (signal) {
        case SIGINT:
            log_message("Finishing up", SILENCE);
            (*map_ptr)[position.x][position.y].traffic--;
            shmdt(map_ptr);
            shmdt(source_list_ptr);
            shmdt(readers);
            msgsnd(id_message_queue_master_taxi, &data_message, sizeof(taxi_data), IPC_NOWAIT);
            log_message("Graceful exit successful", DB);
            print_report();
            exit(EXIT_SUCCESS);

        case SIGQUIT:
            log_message("SIGQUIT", DB);
            break;

        case SIGALRM:
            raise(SIGINT);
            break;

        case SIGUSR1:
            log_message("Received SIGUSR1", DB);
            break;

        case SIGUSR2:
            log_message("Received SIGUSR2", DB);
            break;
    }
}

void log_message(char *message, enum level l) {
    if (l <= DEBUG) {
        printf("[master-%d] %s\n", getpid(), message);
    }
}