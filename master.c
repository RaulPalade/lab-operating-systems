#include "util.h"
#include "master.h"

int main() {
    configuration configuration;
    read_configuration(&configuration);
    print_configuration(configuration);
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