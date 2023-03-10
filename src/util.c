#include "../include/util.h"

long get_timestamp_millis() {
    struct timespec t;
    clock_gettime(CLOCK_REALTIME, &t);
    return t.tv_sec * 1000 + lround(t.tv_nsec / 1e6);
}

int equal_transaction(transaction t1, transaction t2) {
    return t1.timestamp == t2.timestamp && t1.sender == t2.sender && t1.receiver == t2.receiver;
}

int array_contains(transaction *transactions, transaction t) {
    int contains = 0;
    int i;
    for (i = 0; i < SO_BLOCK_SIZE - 1 && !contains; i++) {
        if (equal_transaction(transactions[i], t)) {
            contains = 1;
        }
    }

    return contains;
}

void print_configuration(configuration configuration) {
    printf("SO_USERS_NUM = %d\n", configuration.SO_USERS_NUM);
    printf("SO_NODES_NUM = %d\n", configuration.SO_NODES_NUM);
    printf("SO_REWARD = %d\n", configuration.SO_REWARD);
    printf("SO_MIN_TRANS_GEN_NSEC = %d\n", configuration.SO_MIN_TRANS_GEN_NSEC);
    printf("SO_MAX_TRANS_GEN_NSEC = %d\n", configuration.SO_MAX_TRANS_GEN_NSEC);
    printf("SO_RETRY = %d\n", configuration.SO_RETRY);
    printf("SO_MIN_TRANS_PROC_NSEC = %d\n", configuration.SO_MIN_TRANS_PROC_NSEC);
    printf("SO_MAX_TRANS_PROC_NSEC = %d\n", configuration.SO_MAX_TRANS_PROC_NSEC);
    printf("SO_BUDGET_INIT = %d\n", configuration.SO_BUDGET_INIT);
    printf("SO_SIM_SEC = %d\n", configuration.SO_SIM_SEC);
    printf("SO_FRIENDS_NUM = %d\n", configuration.SO_FRIENDS_NUM);
}

void print_table_header() {
    printf(ANSI_COLOR_GREEN "=============================================================================================================================\n" ANSI_COLOR_RESET);
    printf("%5s %15s %5s %10s %15s %5s %10s %15s %5s %10s %15s %5s %10s %15s\n",
           "",
           "TIMESTAMP", "", ANSI_COLOR_MAGENTA "|" ANSI_COLOR_RESET,
           "SENDER", "", ANSI_COLOR_MAGENTA "|" ANSI_COLOR_RESET,
           "RECEIVER", "", ANSI_COLOR_MAGENTA "|" ANSI_COLOR_RESET,
           "AMOUNT", "", ANSI_COLOR_MAGENTA "|" ANSI_COLOR_RESET,
           "REWARD");
    printf(ANSI_COLOR_GREEN "=============================================================================================================================\n" ANSI_COLOR_RESET);
}

void print_transaction(transaction t) {
    printf("%5s %15ld %5s %10s %15d %5s %10s %15d %5s %10s %15d %5s %10s %15d\n",
           "",
           t.timestamp, "", ANSI_COLOR_MAGENTA "|" ANSI_COLOR_RESET,
           t.sender, "", ANSI_COLOR_MAGENTA "|" ANSI_COLOR_RESET,
           t.receiver, "", ANSI_COLOR_MAGENTA "|" ANSI_COLOR_RESET,
           t.amount, "", ANSI_COLOR_MAGENTA "|" ANSI_COLOR_RESET,
           t.reward);
}

void print_block(block block) {
    int i;
    printf("BLOCK ID: %d\n", block.id);
    print_table_header();
    for (i = 0; i < SO_BLOCK_SIZE; i++) {
        print_transaction(block.transactions[i]);
    }
}

void print_all_transactions(transaction *transactions) {
    int i;
    print_table_header();
    for (i = 0; i < 3; i++) {
        print_transaction(transactions[i]);
    }
    printf("-----------------------------------------------------------------------------------------------------\n");
}