#ifndef __TAXI_H_
#define __TAXI_H_

#include "general.h"
#include <limits.h>
#include <math.h>

void increase_traffic_at_point(map_point);

void decrease_traffic_at_point(map_point);

int can_transit(map_point);

map_point get_near_source(int *);

void check_time_out();

void move_to_point(map_point);

void print_report();

void handler(int);

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
    taxi_data taxi_data;
} taxi_message;

#endif