#ifndef __GENERATOR_H_
#define __GENERATOR_H_

#include "general.h"
#include <stdio.h>

void unblock(int);

void parse_configuration(simulation_configuration *);

int check_no_adjacent_holes(cell (*)[][SO_HEIGHT], int, int);

void generate_map(cell (*)[][SO_HEIGHT], simulation_configuration *);

void print_map(cell (*)[][SO_HEIGHT]);

void execute_source(int);

void execute_taxi();

void handler(int);

#endif