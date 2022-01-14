#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <unistd.h>

#define N 10

typedef struct {
    pid_t pid;
    int balance;
} user_data;

void add_max(user_data *, pid_t, int);

void add_min(user_data *, pid_t, int);

void init_array(user_data *, int);

void print_array(user_data *);

int main() {
    user_data top_list[N];
    user_data min_list[N];
    init_array(top_list, INT_MIN);
    add_max(top_list, 2222, 2);
    add_max(top_list, 3333, 3);
    add_max(top_list, 4444, 4);
    add_max(top_list, 5555, 5);
    add_max(top_list, 6666, 6);
    add_max(top_list, 7777, 7);
    add_max(top_list, 8888, 8);
    add_max(top_list, 9999, 9);
    add_max(top_list, 1000, 10);
    add_max(top_list, 1100, 11);
    add_max(top_list, 1200, 12);
    add_max(top_list, 1300, 13);
    add_max(top_list, 1400, 14);
    printf("\n");
    print_array(top_list);

    printf("\n");
    init_array(min_list, INT_MAX);
    add_min(min_list, 3000, 30);
    add_min(min_list, 2000, 20);
    add_min(min_list, 1000, 10);
    add_min(min_list, 9999, 9);
    add_min(min_list, 8888, 8);
    add_min(min_list, 7777, 7);
    add_min(min_list, 6666, 6);
    add_min(min_list, 5555, 5);
    add_min(min_list, 4444, 4);
    add_min(min_list, 3333, 3);
    add_min(min_list, 2222, 2);
    add_min(min_list, 1111, 1);
    printf("\n");
    print_array(min_list);
    printf("\n");

    return 0;
}

void add_max(user_data *array, pid_t pid, int balance) {
    int i;
    int added = 0;
    for (i = 0; i < N && !added; i++) {
        if (balance > array[i].balance) {
            memmove(array + i + 1, array + i, (N - i - 1) * sizeof(*array));
            array[i].pid = pid;
            array[i].balance = balance;
            added = 1;
        }
    }
}

void add_min(user_data *array, pid_t pid, int balance) {
    int i;
    int added = 0;
    for (i = 0; i < N && !added; i++) {
        if (balance < array[i].balance) {
            memmove(array + i + 1, array + i, (N - i - 1) * sizeof(*array));
            array[i].pid = pid;
            array[i].balance = balance;
            added = 1;
        }
    }
}

void init_array(user_data *array, int value) {
    int i;
    for (i = 0; i < N; i++) {
        array[i].pid = 0;
        array[i].balance = value;
    }
}

void print_array(user_data *array) {
    int i;
    printf("%5s%10s\n", "PID", "BALANCE");
    printf("---------------------------\n");
    for (i = 0; i < N; i++) {
        printf("%5d%10d\n", array[i].pid, array[i].balance);
    }
}