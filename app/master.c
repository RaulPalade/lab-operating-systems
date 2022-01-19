#include "../include/master.h"
#include "../include/siglib.h"

/* CONFIGURATION STRUCTURE */
configuration config;

/* SHARED MEMORY STRUCTURES */
ledger (*master_ledger);
pid_t *user_list;
int *block_id;

/* MESSAGE QUEUE STRUCTURES */
friend_list_message friend_list_msg;    /* USED TO ADD LIST OF FRIENDS TO THE MESSAGE */
tx_message tx_node_master;              /* USED FOR TRANSACTION MESSAGE FROM NODE TO MASTER */
node_txl_message txl_node;              /* USED BY NODE TO SEND TX LEFT AT THE END TO MASTER */

/* SHARED MEMORY ID */
int id_shm_ledger;              /* USED TO STORE THE LEDGER */
int id_shm_user_list;           /* USED TO SHARE USER PID LIST BETWEEN USERS */
int id_shm_block_id;            /*  */

/* MESSAGE QUEUE ID */
int id_msg_tx_node_master;      /* USED TO SEND TRANSACTION WITH MAX HOPS FROM NODE TO MASTER AND TX LEFT */
int id_msg_tx_node_user;        /* USED TO SEND FAILURE TRANSACTION FROM NODE TO USER */
int id_msg_tx_user_node;        /* USED TO SEND NEW TRANSACTION FROM USER TO NODE */
int id_msg_tx_node_friends;     /* USED TO SEND TRANSACTION FROM NODE TO ANOTHER FRIEND NODE */
int id_msg_friend_list;         /* USED TO SEND NODE FRIEND LIST FROM MASTER TO NODE */

/* SEMAPHORE ID */
int id_sem_init;                /* USED TO INIT ALL THE RESOURCES */
int id_sem_block_id;            /* USED TO ACCESS CURRENT BLOCK ID IN LEDGER */

/* SIMULATION RUNNING PARAMETERS */
volatile int executing = 1;
int ledger_full = 0;
int active_nodes = 0;
int active_users = 0;
int final_alive_users = 0;
int new_nodes;
pid_t *node_list;
pid_t *tmp_node_list;

int main() {
    int i;
    int node_i;
    int user_i;
    int node_balance;
    int user_balance;
    int tmp_block_id;
    key_t key;
    pid_t node_pid;
    pid_t user_pid;
    struct sigaction sa;
    struct timespec request;
    struct timespec remaining;
    sigset_t my_mask;

    child_data *balance_nodes;
    child_data *balance_users;
    child_data top_nodes[N_CHILD_TO_DISPLAY];
    child_data top_users[N_CHILD_TO_DISPLAY];
    child_data worst_users[N_CHILD_TO_DISPLAY];

    char *args_node[16] = {NODE};
    char *args_user[15] = {USER};
    char index_node[3 * sizeof(int) + 1];
    char index_user[3 * sizeof(int) + 1];

    char so_tp_size[3 * sizeof(int) + 1];
    char so_min_trans_proc_nsec[3 * sizeof(int) + 1];
    char so_max_trans_proc_nsec[3 * sizeof(int) + 1];
    char so_friends_num[3 * sizeof(int) + 1];
    char so_hops_num[3 * sizeof(int) + 1];

    char so_budget_init[3 * sizeof(int) + 1];
    char so_retry[3 * sizeof(int) + 1];
    char so_min_trans_gen_nsec[3 * sizeof(int) + 1];
    char so_max_trans_gen_nsec[3 * sizeof(int) + 1];
    char so_nodes_num[3 * sizeof(int) + 1];
    char so_users_num[3 * sizeof(int) + 1];
    char so_reward[3 * sizeof(int) + 1];

    char id_shm_ledger_str[3 * sizeof(int) + 1];
    char id_shm_block_id_str[3 * sizeof(int) + 1];
    char id_shm_user_list_str[3 * sizeof(int) + 1];

    char id_msg_tx_node_master_str[3 * sizeof(int) + 1];
    char id_msg_tx_node_user_str[3 * sizeof(int) + 1];
    char id_msg_tx_user_node_str[3 * sizeof(int) + 1];
    char id_msg_tx_node_friends_str[3 * sizeof(int) + 1];
    char id_msg_friend_list_str[3 * sizeof(int) + 1];

    char id_sem_init_str[3 * sizeof(int) + 1];
    char id_sem_block_id_str[3 * sizeof(int) + 1];

    request.tv_sec = 1;
    request.tv_nsec = 0;
    remaining.tv_sec = 1;
    remaining.tv_nsec = 0;

    mask(SIGINT, my_mask);
    mask(SIGTERM, my_mask);
    mask(SIGQUIT, my_mask);
    bzero(&sa, sizeof(sa));
    sa.sa_handler = handler;
    sigaction(SIGALRM, &sa, 0);
    sigaction(SIGINT, &sa, 0);
    sigaction(SIGTERM, &sa, 0);
    sigaction(SIGQUIT, &sa, 0);
    sigaction(SIGCHLD, &sa, 0);
    sigaction(SIGUSR1, &sa, 0);
    sigaction(SIGUSR2, &sa, 0);

    read_configuration(&config);

    node_list = malloc(config.SO_NODES_NUM * sizeof(pid_t));
    tmp_node_list = malloc(config.SO_NODES_NUM * sizeof(pid_t));
    balance_nodes = malloc(config.SO_NODES_NUM * sizeof(child_data));
    balance_users = malloc(config.SO_USERS_NUM * sizeof(child_data));

    /* SHARED MEMORY CREATION */
    key = ftok("../makefile", PROJ_ID_SHM_LEDGER);
    TEST_ERROR
    id_shm_ledger = shmget(key, sizeof(block) * SO_REGISTRY_SIZE, IPC_CREAT | 0666);
    TEST_ERROR
    master_ledger = shmat(id_shm_ledger, NULL, 0);
    TEST_ERROR

    key = ftok("../makefile", PROJ_ID_SHM_USER_LIST);
    TEST_ERROR
    id_shm_user_list = shmget(key, sizeof(pid_t) * config.SO_USERS_NUM, IPC_CREAT | 0666);
    TEST_ERROR
    user_list = shmat(id_shm_user_list, NULL, 0);

    key = ftok("../makefile", PROJ_ID_SHM_BLOCK_ID);
    TEST_ERROR
    id_shm_block_id = shmget(key, sizeof(block_id), IPC_CREAT | 0666);
    TEST_ERROR
    block_id = shmat(id_shm_block_id, NULL, 0);
    TEST_ERROR
    *block_id = 0;

    /* MESSAGE QUEUE CREATION */
    key = ftok("../makefile", PROJ_ID_MSG_NODE_USER);
    TEST_ERROR
    id_msg_tx_node_user = msgget(key, IPC_CREAT | 0666);
    TEST_ERROR

    key = ftok("../makefile", PROJ_ID_MSG_USER_NODE);
    TEST_ERROR
    id_msg_tx_user_node = msgget(key, IPC_CREAT | 0666);
    TEST_ERROR

    key = ftok("../makefile", PROJ_ID_MSG_FRIEND_LIST);
    TEST_ERROR
    id_msg_friend_list = msgget(key, IPC_CREAT | 0666);
    TEST_ERROR

    key = ftok("../makefile", PROJ_ID_MSG_NODE_FRIENDS);
    TEST_ERROR
    id_msg_tx_node_friends = msgget(key, IPC_CREAT | 0666);
    TEST_ERROR

    key = ftok("../makefile", PROJ_ID_MSG_NODE_MASTER);
    TEST_ERROR
    id_msg_tx_node_master = msgget(key, IPC_CREAT | 0666);
    TEST_ERROR

    /* SEMAPHORE CREATION */
    key = ftok("../makefile", PROJ_ID_SEM_INIT);
    TEST_ERROR
    id_sem_init = semget(key, 1, IPC_CREAT | 0666);
    TEST_ERROR
    set_semaphore_val(id_sem_init, 0, 1);

    key = ftok("../makefile", PROJ_ID_SEM_BLOCK_ID);
    TEST_ERROR
    id_sem_block_id = semget(key, 1, IPC_CREAT | 0666);
    TEST_ERROR
    set_semaphore_val(id_sem_block_id, 0, 1);

    unmask(SIGINT, my_mask);
    unmask(SIGTERM, my_mask);
    unmask(SIGINT, my_mask);
    unmask(SIGQUIT, my_mask);

    sprintf(so_tp_size, "%d", config.SO_TP_SIZE);
    sprintf(so_min_trans_proc_nsec, "%d", config.SO_MIN_TRANS_PROC_NSEC);
    sprintf(so_max_trans_proc_nsec, "%d", config.SO_MAX_TRANS_PROC_NSEC);
    sprintf(so_friends_num, "%d", config.SO_FRIENDS_NUM);
    sprintf(so_hops_num, "%d", config.SO_HOPS);

    sprintf(so_budget_init, "%d", config.SO_BUDGET_INIT);
    sprintf(so_retry, "%d", config.SO_RETRY);
    sprintf(so_min_trans_gen_nsec, "%d", config.SO_MIN_TRANS_GEN_NSEC);
    sprintf(so_max_trans_gen_nsec, "%d", config.SO_MAX_TRANS_GEN_NSEC);
    sprintf(so_nodes_num, "%d", config.SO_NODES_NUM);
    sprintf(so_users_num, "%d", config.SO_USERS_NUM);
    sprintf(so_reward, "%d", config.SO_REWARD);

    sprintf(id_shm_ledger_str, "%d", id_shm_ledger);
    sprintf(id_shm_user_list_str, "%d", id_shm_user_list);
    sprintf(id_shm_block_id_str, "%d", id_shm_block_id);

    sprintf(id_msg_tx_node_master_str, "%d", id_msg_tx_node_master);
    sprintf(id_msg_tx_node_user_str, "%d", id_msg_tx_node_user);
    sprintf(id_msg_tx_user_node_str, "%d", id_msg_tx_user_node);
    sprintf(id_msg_tx_node_friends_str, "%d", id_msg_tx_node_friends);
    sprintf(id_msg_friend_list_str, "%d", id_msg_friend_list);

    sprintf(id_sem_init_str, "%d", id_sem_init);
    sprintf(id_sem_block_id_str, "%d", id_sem_block_id);

    args_node[1] = so_tp_size;
    args_node[2] = so_min_trans_proc_nsec;
    args_node[3] = so_max_trans_proc_nsec;
    args_node[4] = so_friends_num;
    args_node[5] = so_hops_num;
    args_node[6] = id_shm_ledger_str;
    args_node[7] = id_shm_block_id_str;
    args_node[8] = id_msg_tx_node_master_str;
    args_node[9] = id_msg_tx_node_user_str;
    args_node[10] = id_msg_tx_user_node_str;
    args_node[11] = id_msg_tx_node_friends_str;
    args_node[12] = id_msg_friend_list_str;
    args_node[13] = id_sem_init_str;
    args_node[14] = id_sem_block_id_str;
    args_node[15] = NULL;

    args_user[1] = so_budget_init;
    args_user[2] = so_retry;
    args_user[3] = so_min_trans_gen_nsec;
    args_user[4] = so_max_trans_gen_nsec;
    args_user[5] = so_users_num;
    args_user[6] = so_reward;
    args_user[7] = id_shm_ledger_str;
    args_user[8] = id_shm_user_list_str;
    args_user[9] = id_shm_block_id_str;
    args_user[10] = id_msg_tx_node_user_str;
    args_user[11] = id_msg_tx_user_node_str;
    args_user[12] = id_sem_init_str;
    args_user[13] = id_sem_block_id_str;
    args_user[14] = NULL;

    /* LAUNCHING NODE PROCESSES */
    for (node_i = 0; node_i < config.SO_NODES_NUM; node_i++) {
        switch (node_pid = fork()) {
            case -1:
                TEST_ERROR

            case 0:
                execv("node", args_node);

            default:
                node_list[node_i] = node_pid;
                tmp_node_list[node_i] = node_pid;
                active_nodes++;
        }
    }

    /* SENDING LIST OF FRIENDS TO ALL NODES */
    for (i = 0; i < config.SO_NODES_NUM; i++) {
        friend_list_msg.mtype = node_list[i];
        memcpy(friend_list_msg.friends, get_random_friends(friend_list_msg.mtype),
               sizeof(pid_t) * config.SO_FRIENDS_NUM);
        msgsnd(id_msg_friend_list, &friend_list_msg, sizeof(friend_list_message), 0);
    }

    /* LAUNCHING USER PROCESSES */
    for (user_i = 0; user_i < config.SO_USERS_NUM; user_i++) {
        switch (user_pid = fork()) {
            case -1:
                TEST_ERROR

            case 0:
                execv("user", args_user);

            default:
                user_list[user_i] = user_pid;
                active_users++;
        }
    }

    init_array(top_users, INT_MIN);
    init_array(worst_users, INT_MAX);
    init_array(top_nodes, INT_MIN);
    alarm(config.SO_SIM_SEC);
    unlock_init_semaphore(id_sem_init);

    delete_shm_segments();

    /* SIMULATION EXECUTION */
    while (executing && !ledger_full && active_users > 0) {
        /* RECEIVING MESSAGES FROM NODE IN CASE OF TX FAIL */
        if (msgrcv(id_msg_tx_node_master, &tx_node_master, sizeof(tx_message), getpid(), IPC_NOWAIT) != -1) {
            /* CREAE NEW PROCESS */
            switch (node_pid = fork()) {
                case -1:
                    TEST_ERROR

                case 0:
                    sprintf(index_node, "%d", node_i);
                    args_node[1] = index_node;
                    execv("node", args_node);

                default:
                    active_nodes++;
                    new_nodes++;
                    node_list = realloc(node_list, sizeof(pid_t) * (config.SO_NODES_NUM + new_nodes));
                    tmp_node_list = realloc(tmp_node_list, sizeof(pid_t) * (config.SO_NODES_NUM + new_nodes));
                    node_list[node_i] = node_pid;
                    tmp_node_list[node_i] = node_pid;
            }

            /* SEND FRIENDS TO NEW PROCESS CREATED */
            friend_list_msg.mtype = node_list[node_i];
            memcpy(friend_list_msg.friends, get_random_friends(friend_list_msg.mtype),
                   sizeof(pid_t) * config.SO_FRIENDS_NUM);
            msgsnd(id_msg_friend_list, &friend_list_msg, sizeof(friend_list_message), 0);

            /* SEND TRANSACTION TO NEW PROCESS CREATED */
            tx_node_master.mtype = node_pid;
            msgsnd(id_msg_tx_node_master, &tx_node_master, sizeof(tx_message), 0);
            node_i++;

            /* SEND CREATED PROCESS TO ALL NODES AS A FRIEND */
            for (i = 0; i < config.SO_NODES_NUM + (new_nodes - 1); i++) {
                friend_list_msg.mtype = node_list[i];
                friend_list_msg.friends[0] = node_pid;
                msgsnd(id_msg_friend_list, &friend_list_msg, sizeof(friend_list_message), 0);
            }
        }

        lock(id_sem_block_id);
        tmp_block_id = *block_id;
        unlock(id_sem_block_id);
        tmp_block_id--;

        /* CALCULATING NODE BALANCE */
        for (i = 0; i < config.SO_NODES_NUM + new_nodes; i++) {
            node_balance = calculate_node_balance(node_list[i], tmp_block_id);
            balance_nodes[i].pid = node_list[i];
            balance_nodes[i].balance = node_balance;
        }

        /* CALCULATING USER BALANCE */
        for (i = 0; i < config.SO_USERS_NUM; i++) {
            user_balance = calculate_user_balance(user_list[i], tmp_block_id);
            balance_users[i].pid = user_list[i];
            balance_users[i].balance = user_balance;
        }

        /* SORTING BALANCE LISTS */
        qsort(balance_nodes, config.SO_NODES_NUM + new_nodes, sizeof(child_data), compare);
        qsort(balance_users, config.SO_USERS_NUM, sizeof(child_data), compare);
        memcpy(top_nodes, balance_nodes, sizeof(child_data) * N_CHILD_TO_DISPLAY);
        memcpy(top_users, balance_users, sizeof(child_data) * N_CHILD_TO_DISPLAY);
        memcpy(worst_users, balance_users + config.SO_USERS_NUM - N_CHILD_TO_DISPLAY,
               sizeof(child_data) * N_CHILD_TO_DISPLAY);

        print_live_info(top_nodes, top_users, worst_users);

        nanosleep(&request, &remaining);
    }

    kill(0, SIGTERM);

    printf("\n");
    printf("\n");
    printf("\n");
    print_ledger();

    /* PRINTING FINAL BALANCES NODES */
    printf("\n%s\n", ANSI_COLOR_GREEN "=============NODES=============" ANSI_COLOR_RESET);
    printf("%8s        %5s    %8s\n", "PID", ANSI_COLOR_MAGENTA "|" ANSI_COLOR_RESET, "BALANCE");
    printf("%s\n", ANSI_COLOR_GREEN "===============================" ANSI_COLOR_RESET);
    for (i = 0; i < config.SO_NODES_NUM + new_nodes; i++) {
        node_balance = calculate_node_balance(node_list[i], *block_id);
        printf("%8d    %5s    %8d\n", node_list[i], "|", node_balance);
    }

    /* PRINTING FINAL BALANCES USERS */
    printf("\n%s\n", ANSI_COLOR_GREEN "=============USERS=============" ANSI_COLOR_RESET);
    printf("%8s        %5s    %8s\n", "PID", ANSI_COLOR_MAGENTA "|" ANSI_COLOR_RESET, "BALANCE");
    printf("%s\n", ANSI_COLOR_GREEN "===============================" ANSI_COLOR_RESET);
    for (i = 0; i < config.SO_USERS_NUM; i++) {
        user_balance = calculate_user_balance(user_list[i], *block_id);
        printf("%8d    %5s    %8d\n", user_list[i], "|", user_balance);
    }

    print_final_report();

    free(node_list);
    free(tmp_node_list);
    free(balance_nodes);
    free(balance_users);

    clean_IPCS();

    return 0;
}

int calculate_node_balance(pid_t node, int tmp_block_id) {
    int node_balance = 0;
    int i;
    int j = SO_BLOCK_SIZE - 1;

    for (i = 0; i < tmp_block_id; i++) {
        if (master_ledger->blocks[i].transactions[j].receiver == node) {
            node_balance += master_ledger->blocks[i].transactions[j].amount;
        }
    }

    return node_balance;
}

int calculate_user_balance(pid_t user, int tmp_block_id) {
    int user_balance = config.SO_BUDGET_INIT;
    int i;
    int j;

    for (i = 0; i < tmp_block_id; i++) {
        for (j = 0; j < SO_BLOCK_SIZE - 1; j++) {
            if (master_ledger->blocks[i].transactions[j].sender == user) {
                user_balance -= master_ledger->blocks[i].transactions[j].amount;
                user_balance -= master_ledger->blocks[i].transactions[j].reward;
            } else if (master_ledger->blocks[i].transactions[j].receiver == user) {
                user_balance += master_ledger->blocks[i].transactions[j].amount;
            }
        }
    }

    return user_balance;
}

void init_array(child_data *array, int value) {
    int i;
    for (i = 0; i < N_CHILD_TO_DISPLAY; i++) {
        array[i].pid = 0;
        array[i].balance = value;
    }
}

void shuffle(int *array) {
    size_t i;
    size_t j;
    int tmp;
    struct timespec tp;
    clock_gettime(CLOCK_REALTIME, &tp);
    srand(tp.tv_nsec);
    if (config.SO_NODES_NUM > 1) {
        for (i = 0; i < (config.SO_NODES_NUM - 1) + new_nodes; i++) {
            j = i + rand() / (RAND_MAX / ((config.SO_NODES_NUM - i) + new_nodes) + 1);
            tmp = array[j];
            array[j] = array[i];
            array[i] = tmp;
        }
    }
}

pid_t *get_random_friends(pid_t node) {
    int i;
    int added = 0;
    pid_t *friend_list = malloc(config.SO_FRIENDS_NUM * sizeof(pid_t));
    shuffle(tmp_node_list);

    for (i = 0; i < config.SO_NODES_NUM + new_nodes && added < config.SO_FRIENDS_NUM; i++) {
        if (tmp_node_list[i] != node) {
            friend_list[added++] = tmp_node_list[i];
        }
    }

    return friend_list;
}

int compare(const void *s1, const void *s2) {
    const child_data *c1 = (child_data *) s1;
    const child_data *c2 = (child_data *) s2;
    if (c1->balance < c2->balance) {
        return 1;
    } else if (c1->balance > c2->balance) {
        return -1;
    } else {
        return 0;
    }
}

void check_deadlock() {
    union semun sem_ds;
    int sem_value;
    sem_ds.val = 0;
    sem_value = semctl(id_sem_block_id, 0, GETVAL, sem_ds.val);
    if (sem_value == 0) {
        unlock(id_sem_block_id);
    }
}

void read_configuration(configuration *config) {
    FILE *file;
    char s[23];
    char comment;
    char filename[] = "../configurations/configuration2.conf";
    int value;
    int i;

    if (access(filename, F_OK) == 0) {
        file = fopen(filename, "r");

        while (fscanf(file, "%s", s) == 1) {
            switch (s[0]) {
                case '#':
                    do {
                        comment = fgetc(file);
                    } while (comment != '\n');
                    break;

                default:
                    fscanf(file, "%d\n", &value);
                    if (value < 0) {
                        printf("value < 0\n");
                        kill(0, SIGINT);
                    }
                    if (strncmp(s, "SO_USERS_NUM", 12) == 0) {
                        config->SO_USERS_NUM = value;
                    } else if (strncmp(s, "SO_NODES_NUM", 12) == 0) {
                        config->SO_NODES_NUM = value;
                    } else if (strncmp(s, "SO_REWARD", 9) == 0) {
                        config->SO_REWARD = value;
                    } else if (strncmp(s, "SO_MIN_TRANS_GEN_NSEC", 21) == 0) {
                        config->SO_MIN_TRANS_GEN_NSEC = value;
                    } else if (strncmp(s, "SO_MAX_TRANS_GEN_NSEC", 21) == 0) {
                        config->SO_MAX_TRANS_GEN_NSEC = value;
                    } else if (strncmp(s, "SO_RETRY", 8) == 0) {
                        config->SO_RETRY = value;
                    } else if (strncmp(s, "SO_TP_SIZE", 10) == 0) {
                        config->SO_TP_SIZE = value;
                    } else if (strncmp(s, "SO_MIN_TRANS_PROC_NSEC", 22) == 0) {
                        config->SO_MIN_TRANS_PROC_NSEC = value;
                    } else if (strncmp(s, "SO_MAX_TRANS_PROC_NSEC", 22) == 0) {
                        config->SO_MAX_TRANS_PROC_NSEC = value;
                    } else if (strncmp(s, "SO_BUDGET_INIT", 14) == 0) {
                        config->SO_BUDGET_INIT = value;
                    } else if (strncmp(s, "SO_SIM_SEC", 10) == 0) {
                        config->SO_SIM_SEC = value;
                    } else if (strncmp(s, "SO_FRIENDS_NUM", 14) == 0) {
                        config->SO_FRIENDS_NUM = value;
                    } else if (strncmp(s, "SO_HOPS", 7) == 0) {
                        config->SO_HOPS = value;
                    }
                    i++;
            }
        }
    } else {
        printf("File doesn't exists\n");
        TEST_ERROR
    }

    if (i < 10) {
        printf("Configuration not valid\n");
        kill(0, SIGINT);
    }

    fclose(file);
}

void delete_shm_segments() {
    shmctl(id_shm_ledger, IPC_RMID, NULL);
    shmctl(id_shm_user_list, IPC_RMID, NULL);
    shmctl(id_shm_block_id, IPC_RMID, NULL);
}

void clean_IPCS() {
    shmdt(master_ledger);
    shmdt(user_list);
    shmdt(block_id);

    msgctl(id_msg_tx_node_master, IPC_RMID, NULL);
    msgctl(id_msg_tx_node_user, IPC_RMID, NULL);
    msgctl(id_msg_tx_user_node, IPC_RMID, NULL);
    msgctl(id_msg_tx_node_friends, IPC_RMID, NULL);
    msgctl(id_msg_friend_list, IPC_RMID, NULL);

    semctl(id_sem_init, 0, IPC_RMID);
    semctl(id_sem_block_id, 0, IPC_RMID);
}

void print_live_info(child_data *top_nodes, child_data *top_users, child_data *worst_users) {
    int i;
    int j;
    printf("\n%10s        %10s        %10s        %10s         %10s\n",
           ANSI_COLOR_GREEN "==========TOP NODES==========" ANSI_COLOR_RESET,
           ANSI_COLOR_GREEN "==========TOP USERS==========" ANSI_COLOR_RESET,
           ANSI_COLOR_GREEN "=========WORST USERS=========" ANSI_COLOR_RESET,
           ANSI_COLOR_GREEN "==================" ANSI_COLOR_RESET,
           ANSI_COLOR_GREEN "==================" ANSI_COLOR_RESET);

    printf("%8s      %5s    %8s          %8s      %5s    %8s         %8s     %5s    %8s               %10s               %10s\n",
           "PID", ANSI_COLOR_MAGENTA "|" ANSI_COLOR_RESET, "BALANCE",
           "PID", ANSI_COLOR_MAGENTA "|" ANSI_COLOR_RESET, "BALANCE",
           "PID", ANSI_COLOR_MAGENTA "|" ANSI_COLOR_RESET, "BALANCE",
           "ACTIVE NODES",
           "ACTIVE USERS");
    printf(ANSI_COLOR_GREEN "=============================        =============================        =============================        ==================         ==================\n" ANSI_COLOR_RESET);

    for (i = 0, j = N_CHILD_TO_DISPLAY - 1; i < N_CHILD_TO_DISPLAY && j >= 0; i++, j--) {
        if (i == 0) {
            printf("%8d      %5s  %8d            %8d      %5s  %8d            %8d    %5s  %8d             %10d                   %10d\n",
                   top_nodes[i].pid, ANSI_COLOR_MAGENTA "|" ANSI_COLOR_RESET, top_nodes[i].balance,
                   top_users[i].pid, ANSI_COLOR_MAGENTA "|" ANSI_COLOR_RESET, top_users[i].balance,
                   worst_users[j].pid, ANSI_COLOR_MAGENTA "|" ANSI_COLOR_RESET, worst_users[j].balance,
                   active_nodes,
                   active_users
            );
        } else {
            printf("%8d      %5s  %8d            %8d      %5s  %8d            %8d    %5s  %8d             %10s                   %10s\n",
                   top_nodes[i].pid, ANSI_COLOR_MAGENTA "|" ANSI_COLOR_RESET, top_nodes[i].balance,
                   top_users[i].pid, ANSI_COLOR_MAGENTA "|" ANSI_COLOR_RESET, top_users[i].balance,
                   worst_users[j].pid, ANSI_COLOR_MAGENTA "|" ANSI_COLOR_RESET, worst_users[j].balance,
                   "",
                   ""
            );
        }
    }
}

void print_ledger() {
    int i;
    int tmp_block_id;
    lock(id_sem_block_id);
    tmp_block_id = *block_id;
    unlock(id_sem_block_id);
    printf(ANSI_COLOR_GREEN "============================================================LEDGER============================================================\n" ANSI_COLOR_RESET);
    for (i = 0; i < tmp_block_id; i++) {
        print_block(master_ledger->blocks[i]);
        printf(ANSI_COLOR_GREEN "=============================================================================================================================\n" ANSI_COLOR_RESET);
    }
}

void print_final_report() {
    int i;
    printf(ANSI_COLOR_CYAN "\n\n==================SIMULATION REPORT==================\n" ANSI_COLOR_RESET);
    printf(ANSI_COLOR_CYAN "=====================================================\n" ANSI_COLOR_RESET);
    printf("Reason of ending: ");
    if (executing == 0) {
        printf("%s", "time expired");
    }
    if (ledger_full == 1) {
        printf("%s", ", ledger full");
    }
    if (active_users == 0) {
        printf("%s", ", all users are dead");
    }
    printf("\nUser processes dead = %d\n", config.SO_USERS_NUM - final_alive_users);
    printf("Number of blocks in the ledger = %d\n", *block_id);
    for (i = 0; i < config.SO_NODES_NUM + new_nodes; i++) {
        msgrcv(id_msg_tx_node_master, &txl_node, sizeof(txl_node), getpid(), 0);
        printf("Node %d transaction left: %d\n", txl_node.n.pid, txl_node.n.transactions_left);
    }
    printf(ANSI_COLOR_CYAN "=====================================================\n" ANSI_COLOR_RESET);
    printf(ANSI_COLOR_CYAN "================SIMULATION TERMINATED================\n" ANSI_COLOR_RESET);
}

void handler(int signal) {
    switch (signal) {
        case SIGALRM:
            final_alive_users = active_users;
            executing = 0;
            check_deadlock();
            break;

        case SIGINT:
            final_alive_users = active_users;
            executing = 0;
            check_deadlock();
            break;

        case SIGTERM:
            final_alive_users = active_users;
            executing = 0;
            check_deadlock();
            break;

        case SIGQUIT:
            final_alive_users = active_users;
            executing = 0;
            check_deadlock();
            break;

        case SIGUSR1:
            active_users--;
            break;

        case SIGUSR2:
            active_nodes--;
            break;

        default:
            break;
    }
}