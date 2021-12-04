#include "util.h"
#include "master.h"

int ledger_size = 0;
int block_size = 0;
ledger master_ledger;

int main() {
    int i;
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
    block.transactions[0] = t1;
    block.transactions[1] = t2;
    block.transactions[2] = t3;    
    add_to_ledger(block);
    

    /* printf("PRINTING ALL TRANSACTIONS\n");
    transaction transactions[] = {t1, t2, t3};
    print_all_transactions(transactions);
    printf("\n"); */

    /* printf("PRINT SINGLE BLOCK\n");
    print_block(&block);
    printf("\n"); */

    printf("PRINTING LEDGER\n");
    print_ledger(&master_ledger);
    printf("\n");

    /* printf("PRINTING CONFIGURATION\n");
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
    printf("SO_TP_SIZE = %d\n", configuration.SO_TP_SIZE);
    printf("SO_MIN_TRANS_PROC_NSEC = %d\n", configuration.SO_MIN_TRANS_PROC_NSEC);
    printf("SO_MAX_TRANS_PROC_NSEC = %d\n", configuration.SO_MAX_TRANS_PROC_NSEC);
    printf("SO_BUDGET_INIT = %d\n", configuration.SO_BUDGET_INIT);
    printf("SO_SIM_SEC = %d\n", configuration.SO_SIM_SEC);
    printf("SO_FRIENDS_NUM = %d\n", configuration.SO_FRIENDS_NUM);
}

void print_transaction(transaction *t) {
    char *status = get_status(*t);
    printf("%15ld %15d %15d %15d %15d %24s\n", t->timestamp, t->sender, t->receiver, t->amount, t->reward, status);
}

void print_all_transactions(transaction *transactions) {
    int i;
    print_table_header();
    for (i = 0; i < 3; i++) {
        print_transaction(&transactions[i]);
    }
    printf("-----------------------------------------------------------------------------------------------------\n");
}

void print_block(block *block) {
    int i;
    printf("BLOCK ID: %d\n", block->id);
    print_table_header();
    for (i = 0; i < SO_BLOCK_SIZE; i++) {
        print_transaction(&block->transactions[i]);
    }
}

void print_ledger(ledger *ledger) {
    int i;
    for (i = 0; i < ledger_size; i++) {
        print_block(&ledger->blocks[i]);
        printf("-----------------------------------------------------------------------------------------------------\n");
        if(&ledger->blocks[i] == NULL) {
            printf("exit\n");
            break;
        }
    }
    printf("-----------------------------------------------------------------------------------------------------\n");
}

void init_ledger() {

}

void execute_node() {

}

void execute_user() {

}

void print_live_ledger_info(ledger *ledger) {

}

void print_final_report() {

}

void handler(int signal) {

}

char * get_status(transaction t){
    char *transaction_status;
    if (t.status == PROCESSING) {
        transaction_status = ANSI_COLOR_YELLOW "PROCESSING" ANSI_COLOR_RESET;
    }
    if (t.status == COMPLETED) {
        transaction_status = ANSI_COLOR_GREEN "COMPLETED" ANSI_COLOR_RESET;
    }
    if (t.status == ABORTED) {
        transaction_status = ANSI_COLOR_RED "ABORTED" ANSI_COLOR_RESET;
    }

    return transaction_status;
}

void print_table_header() {
    printf("-----------------------------------------------------------------------------------------------------\n");
    printf("%15s %15s %15s %15s %15s %15s\n", "TIMESTAMP", "SENDER", "RECEIVER", "AMOUNT", "REWARD", "STATUS");
    printf("-----------------------------------------------------------------------------------------------------\n");
}

int add_to_block(block *block, transaction *transaction) {
    
}

int add_to_ledger(block block) {
    int added = 0;
    if (ledger_size < SO_REGISTRY_SIZE) {
        master_ledger.blocks[ledger_size] = block;
        ledger_size++;
        added = 1;
    } else {
        printf(ANSI_COLOR_RED "Ledger size exceeded\n" ANSI_COLOR_RESET);
    }

    return added;
}