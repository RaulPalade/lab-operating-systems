#include "util.h"
#include "master.h"

static int ledger_size = 0;
static int block_size = 0;
ledger master_ledger;

/**
 * MASTER PROCESS
 * 1) Acquire general semaphore to init resources
 * 2) Read configuration
 * 3) Init ledger
 * 4) Init nodes
 * 5) Init users
 * 6) Release general semaphore 
 * 7) Print node and user budget each second
 * 8) Stop all nodes and users at the end of the simulation
 * 9) Print final report
 */
int main() {
/*     int i;
    block block;

    transaction t1;
    t1.timestamp = time(NULL);
    t1.sender = 111111;
    t1.receiver = 222222;
    t1.amount = 20;
    t1.reward = 5;
    t1.status = PROCESSING;

    transaction t2;
    t2.timestamp = time(NULL);
    t2.sender = 333333;
    t2.receiver = 444444;
    t2.amount = 60;
    t2.reward = 15;
    t2.status = COMPLETED;

    transaction t3;
    t3.timestamp = time(NULL);
    t3.sender = 555555;
    t3.receiver = 666666;
    t3.amount = 30;
    t3.reward = 10;
    t3.status = ABORTED;

    block.id = 0;
    add_to_block(&block, t1);
    add_to_block(&block, t2);
    add_to_block(&block, t3);

    print_block(&block);
    printf("Printing bloc after removing t1\n");
    remove_from_block(&block, t3);
    printf("Block SIZE = %d\n", block_size);
    print_block(&block); */


    /* add_to_ledger(block);
    print_ledger(&master_ledger);
    printf("Printing after removing block from ledger\n");
    remove_from_ledger(block);
    print_ledger(&master_ledger);

    printf("PRINTING ALL TRANSACTIONS\n");
    transaction transactions[] = {t1, t2, t3};
    print_all_transactions(transactions);
    printf("\n");

    printf("PRINT SINGLE BLOCK\n");
    print_block(&block);
    printf("\n");

    printf("PRINTING LEDGER\n");
    print_ledger(&master_ledger);
    printf("\n");

    printf("PRINTING CONFIGURATION\n");
    configuration configuration;
    read_configuration(&configuration);
    print_configuration(configuration);
    printf("\n"); */
    return 0;
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

void print_all_transactions(transaction *transactions) {
    int i;
    print_table_header();
    for (i = 0; i < 3; i++) {
        print_transaction(transactions[i]);
    }
    printf("-----------------------------------------------------------------------------------------------------\n");
}

void print_block(block *block) {
    int i;
    printf("BLOCK ID: %d\n", block->id);
    print_table_header();
    for (i = 0; i < block_size; i++) {
        print_transaction(block->transactions[i]);
    }
}

void print_ledger(ledger *ledger) {
    int i;
    for (i = 0; i < ledger_size; i++) {
        print_block(&ledger->blocks[i]);
        printf("-----------------------------------------------------------------------------------------------------\n");
    }
    printf("-----------------------------------------------------------------------------------------------------\n");
}

void init_ledger() {

}

void execute_node() {

}

void execute_user() {

}

void print_live_ledger_info() {

}

void print_final_report() {

}

void handler(int signal) {

}