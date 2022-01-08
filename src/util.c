#include "../include/util.h"

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

int equal_transaction(transaction t1, transaction t2) {
    return t1.timestamp == t2.timestamp && t1.sender == t2.sender && t1.receiver == t2.receiver;
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
    printf("-----------------------------------------------------------------------------------------------------\n");
    printf("%15s %15s %15s %15s %15s\n", "TIMESTAMP", "SENDER", "RECEIVER", "AMOUNT", "REWARD");
    printf("-----------------------------------------------------------------------------------------------------\n");
}

void print_transaction(transaction t) {
    printf("%15ld %15d %15d %15ld %15ld\n", t.timestamp, t.sender, t.receiver, t.amount, t.reward);
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