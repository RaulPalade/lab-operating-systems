#ifndef __MASTER_H_
#define __MASTER_H_

#include "general.h"
#include <stdlib.h>
#include <string.h>

typedef struct {
    int distance;
    int longest_travel;
    int clients;
    int success_travel;
    int abort;
    struct timeval max_travel_time;
} taxi_data;

typedef struct {
    long type;
    int requests;
} source_message;

typedef struct {
    long type;
    taxi_data taxi_data;
} taxi_message;

typedef struct {
    int requests;
    int total_travels;
    int success_travels;
    int not_served_travels;

    int max_travels;
    long travels_winner;

    struct timeval max_time;
    long time_winner;

    int max_distance;
    long distance_winner;

    int top_cells;
    map_point (*winner_cells)[];
} simulation_data;

void gathering_cells_data(cell (*)[][SO_HEIGHT], int);

void update_cells_data(long, taxi_data *);

void print_map(cell (*)[][SO_HEIGHT]);

void print_map_report(cell (*)[][SO_HEIGHT]);

void handler(int);

#endif