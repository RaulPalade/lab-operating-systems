#include "../include/master.h"
#include "../include/siglib.h"

#define N_BALANCE_PRINT 10

configuration config;
ledger (*master_ledger);
pid_t *user_list;
int *block_id;
int *readers_block_id;
master_node_fl_message master_node_fl_msg;
node_master_message node_master_msg;
master_node_new_friend_message master_node_new_friend_msg;

/* SHARED MEMORY */
int id_shm_ledger;
int id_shm_user_list;
int id_shm_block_id;
int id_shm_readers_block_id;

/* MESSAGE QUEUE */
int id_msg_node_user;
int id_msg_user_node;
int id_msg_master_node_friend_list;
int id_msg_node_friends;
int id_msg_node_master;
int id_msg_master_node_new_friend;

/* SEMAPHORE */
int id_sem_init;
int id_sem_writers_block_id;
int id_sem_readers_block_id;

/* SIMULATION RUNNING PARAMETERS */
volatile int executing = 1;
int ledger_full = 0;
int active_nodes = 0;
int active_users = 0;
int final_alive_users = 0;

int initial_total_funds;
int final_total_funds;
int new_nodes;

pid_t *node_list;
pid_t *tmp_node_list;

/**
 * MASTER PROCESS
 * 1) Acquire general semaphore to init resources
 * 2) Read configuration
 * 3) Init nodes => assign initial budget through args in execv
 * 4) Init users
 * 5) Release general semaphore 
 * 6) Print node and user budget each second
 * 7) Stop all nodes and users at the end of the simulation
 * 8) Print final report
 */
int main() {
    int i;
    int j;
    int node_i;
    int user_i;
    int node_balance;
    int user_balance;
    int tmp_block_id;
    int *random_index;

    child_data top_nodes[N_USER_TO_DISPLAY];
    child_data top_users[N_USER_TO_DISPLAY];
    child_data worst_users[N_USER_TO_DISPLAY];

    char *args_node[18] = {NODE};
    char *args_user[17] = {USER};
    char index_node[3 * sizeof(int) + 1];
    char index_user[3 * sizeof(int) + 1];

    char so_tp_size[3 * sizeof(int) + 1];
    char so_min_trans_proc_nsec[3 * sizeof(int) + 1];
    char so_max_trans_proc_nsec[3 * sizeof(int) + 1];
    char so_friends_num[3 * sizeof(int) + 1];
    char so_hopes_num[3 * sizeof(int) + 1];

    char so_budget_init[3 * sizeof(int) + 1];
    char so_retry[3 * sizeof(int) + 1];
    char so_min_trans_gen_nsec[3 * sizeof(int) + 1];
    char so_max_trans_gen_nsec[3 * sizeof(int) + 1];
    char so_nodes_num[3 * sizeof(int) + 1];
    char so_users_num[3 * sizeof(int) + 1];

    char id_shm_ledger_str[3 * sizeof(int) + 1];
    char id_shm_block_id_str[3 * sizeof(int) + 1];
    char id_shm_readers_block_id_str[3 * sizeof(int) + 1];
    char id_shm_user_list_str[3 * sizeof(int) + 1];
    char id_msg_node_user_str[3 * sizeof(int) + 1];
    char id_msg_user_node_str[3 * sizeof(int) + 1];
    char id_msg_master_node_nf_str[3 * sizeof(int) + 1];
    char id_msg_node_friends_str[3 * sizeof(int) + 1];
    char id_msg_node_master_str[3 * sizeof(int) + 1];
    char id_msg_master_node_new_friend_str[3 * sizeof(int) + 1];
    char id_sem_init_str[3 * sizeof(int) + 1];
    char id_sem_writers_block_id_str[3 * sizeof(int) + 1];
    char id_sem_readers_block_id_str[3 * sizeof(int) + 1];
    key_t key;
    pid_t node_pid;
    pid_t user_pid;
    struct sigaction sa;
    struct timespec request;
    struct timespec remaining;
    sigset_t my_mask;
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

    /* SHARED MEMORY CREATION */
    key = ftok("../makefile", PROJ_ID_SHM_LEDGER);
    EXIT_ON_ERROR
    id_shm_ledger = shmget(key, sizeof(block) * SO_REGISTRY_SIZE, IPC_CREAT | 0666);
    EXIT_ON_ERROR
    master_ledger = shmat(id_shm_ledger, NULL, 0);
    EXIT_ON_ERROR

    key = ftok("../makefile", PROJ_ID_SHM_USER_LIST);
    EXIT_ON_ERROR
    id_shm_user_list = shmget(key, sizeof(pid_t) * config.SO_USERS_NUM, IPC_CREAT | 0666);
    EXIT_ON_ERROR
    user_list = shmat(id_shm_user_list, NULL, 0);

    key = ftok("../makefile", PROJ_ID_SHM_BLOCK_ID);
    EXIT_ON_ERROR
    id_shm_block_id = shmget(key, sizeof(block_id), IPC_CREAT | 0666);
    EXIT_ON_ERROR
    block_id = shmat(id_shm_block_id, NULL, 0);
    EXIT_ON_ERROR
    *block_id = 0;

    key = ftok("../makefile", PROJ_ID_SHM_READERS_BLOCK_ID);
    EXIT_ON_ERROR
    id_shm_readers_block_id = shmget(key, sizeof(readers_block_id), IPC_CREAT | 0666);
    EXIT_ON_ERROR
    readers_block_id = shmat(id_shm_readers_block_id, NULL, 0);
    EXIT_ON_ERROR
    *readers_block_id = 0;

    /* MESSAGE QUEUE CREATION */
    key = ftok("../makefile", PROJ_ID_MSG_NODE_USER);
    EXIT_ON_ERROR
    id_msg_node_user = msgget(key, IPC_CREAT | 0666);
    EXIT_ON_ERROR

    key = ftok("../makefile", PROJ_ID_MSG_USER_NODE);
    EXIT_ON_ERROR
    id_msg_user_node = msgget(key, IPC_CREAT | 0666);
    EXIT_ON_ERROR

    key = ftok("../makefile", PROJ_ID_MSG_MASTER_NODE_NF);
    EXIT_ON_ERROR
    id_msg_master_node_friend_list = msgget(key, IPC_CREAT | 0666);
    EXIT_ON_ERROR

    key = ftok("../makefile", PROJ_ID_MSG_NODE_FRIENDS);
    EXIT_ON_ERROR
    id_msg_node_friends = msgget(key, IPC_CREAT | 0666);
    EXIT_ON_ERROR

    key = ftok("../makefile", PROJ_ID_MSG_NODE_MASTER);
    EXIT_ON_ERROR
    id_msg_node_master = msgget(key, IPC_CREAT | 0666);
    EXIT_ON_ERROR

    key = ftok("../makefile", PROJ_ID_MSG_MASTER_NEW_FRIEND);
    EXIT_ON_ERROR
    id_msg_master_node_new_friend = msgget(key, IPC_CREAT | 0666);
    EXIT_ON_ERROR

    /* SEMAPHORE CREATION */
    key = ftok("../makefile", PROJ_ID_SEM_INIT);
    EXIT_ON_ERROR
    id_sem_init = semget(key, 1, IPC_CREAT | 0666);
    EXIT_ON_ERROR
    set_semaphore_val(id_sem_init, 0, 1);

    key = ftok("../makefile", PROJ_ID_SEM_WRITERS);
    EXIT_ON_ERROR
    id_sem_writers_block_id = semget(key, 1, IPC_CREAT | 0666);
    EXIT_ON_ERROR
    set_semaphore_val(id_sem_writers_block_id, 0, 1);

    key = ftok("../makefile", PROJ_ID_SEM_READERS_BLOCK_ID);
    EXIT_ON_ERROR
    id_sem_readers_block_id = semget(key, 1, IPC_CREAT | 0666);
    EXIT_ON_ERROR
    set_semaphore_val(id_sem_readers_block_id, 0, 1);

    unmask(SIGINT, my_mask);
    unmask(SIGTERM, my_mask);
    unmask(SIGINT, my_mask);
    unmask(SIGQUIT, my_mask);

    /* Preparing cmd line arguments for execv */
    sprintf(so_tp_size, "%d", config.SO_TP_SIZE);
    sprintf(so_min_trans_proc_nsec, "%d", config.SO_MIN_TRANS_PROC_NSEC);
    sprintf(so_max_trans_proc_nsec, "%d", config.SO_MAX_TRANS_PROC_NSEC);
    sprintf(so_friends_num, "%d", config.SO_FRIENDS_NUM);
    sprintf(so_hopes_num, "%d", config.SO_HOPS);

    sprintf(so_budget_init, "%d", config.SO_BUDGET_INIT);
    sprintf(so_retry, "%d", config.SO_RETRY);
    sprintf(so_min_trans_gen_nsec, "%d", config.SO_MIN_TRANS_GEN_NSEC);
    sprintf(so_max_trans_gen_nsec, "%d", config.SO_MAX_TRANS_GEN_NSEC);
    sprintf(so_nodes_num, "%d", config.SO_NODES_NUM);
    sprintf(so_users_num, "%d", config.SO_USERS_NUM);

    sprintf(id_shm_ledger_str, "%d", id_shm_ledger);
    sprintf(id_shm_user_list_str, "%d", id_shm_user_list);
    sprintf(id_shm_block_id_str, "%d", id_shm_block_id);
    sprintf(id_shm_readers_block_id_str, "%d", id_shm_readers_block_id);

    sprintf(id_msg_node_user_str, "%d", id_msg_node_user);
    sprintf(id_msg_user_node_str, "%d", id_msg_user_node);
    sprintf(id_msg_master_node_nf_str, "%d", id_msg_master_node_friend_list);
    sprintf(id_msg_node_friends_str, "%d", id_msg_node_friends);
    sprintf(id_msg_node_master_str, "%d", id_msg_node_master);
    sprintf(id_msg_master_node_new_friend_str, "%d", id_msg_master_node_new_friend);

    sprintf(id_sem_init_str, "%d", id_sem_init);
    sprintf(id_sem_writers_block_id_str, "%d", id_sem_writers_block_id);
    sprintf(id_sem_readers_block_id_str, "%d", id_sem_readers_block_id);

    args_node[2] = so_tp_size;
    args_node[3] = so_min_trans_proc_nsec;
    args_node[4] = so_max_trans_proc_nsec;
    args_node[5] = id_shm_ledger_str;
    args_node[6] = id_shm_block_id_str;
    args_node[7] = id_msg_node_user_str;
    args_node[8] = id_msg_user_node_str;
    args_node[9] = id_sem_init_str;
    args_node[10] = id_sem_writers_block_id_str;
    args_node[11] = so_friends_num;
    args_node[12] = id_msg_master_node_nf_str;
    args_node[13] = id_msg_node_friends_str;
    args_node[14] = so_hopes_num;
    args_node[15] = id_msg_node_master_str;
    args_node[16] = id_msg_master_node_new_friend_str;
    args_node[17] = NULL;

    args_user[2] = so_budget_init;
    args_user[3] = so_retry;
    args_user[4] = so_min_trans_gen_nsec;
    args_user[5] = so_max_trans_gen_nsec;
    args_user[6] = so_users_num;
    args_user[7] = id_shm_ledger_str;
    args_user[8] = id_shm_user_list_str;
    args_user[9] = id_shm_block_id_str;
    args_user[10] = id_shm_readers_block_id_str;
    args_user[11] = id_msg_node_user_str;
    args_user[12] = id_msg_user_node_str;
    args_user[13] = id_sem_init_str;
    args_user[14] = id_sem_writers_block_id_str;
    args_user[15] = id_sem_readers_block_id_str;
    args_user[16] = NULL;

    for (node_i = 0; node_i < config.SO_NODES_NUM; node_i++) {
        switch (node_pid = fork()) {
            case -1:
                EXIT_ON_ERROR

            case 0:
                sprintf(index_node, "%d", node_i);
                args_node[1] = index_node;
                execv("node", args_node);

            default:
                node_list[node_i] = node_pid;
                tmp_node_list[node_i] = node_pid;
                active_nodes++;
        }
    }

    for (i = 0; i < config.SO_NODES_NUM; i++) {
        master_node_fl_msg.mtype = node_list[i];
        /*master_node_fl_msg.friends = get_random_friends(tmp_node_list[i]);*/
        master_node_fl_msg.friends[0] = 1111;
        master_node_fl_msg.friends[1] = 2222;
        master_node_fl_msg.friends[2] = 3333;
        master_node_fl_msg.friends[3] = 4444;
        master_node_fl_msg.friends[4] = 5555;
        master_node_fl_msg.friends[5] = 6666;
        /*for (j = 0; j < config.SO_FRIENDS_NUM; j++) {
            printf("%ld\n", master_node_fl_msg.mtype);
        }*/
        msgsnd(id_msg_master_node_friend_list, &master_node_fl_msg, sizeof(master_node_fl_message), 0);
        /*msgrcv(id_msg_master_node_friend_list, &master_node_fl_msg, sizeof(master_node_fl_message), 1,
               IPC_NOWAIT);
        printf("master_node_fl_msg=%d\n", master_node_fl_msg.friends[0]);*/
    }

    for (user_i = 0; user_i < config.SO_USERS_NUM; user_i++) {
        switch (user_pid = fork()) {
            case -1:
                EXIT_ON_ERROR

            case 0:
                sprintf(index_user, "%d", user_i);
                args_user[1] = index_user;
                execv("user", args_user);

            default:
                user_list[user_i] = user_pid;
                initial_total_funds += 1000;
                active_users++;
        }
    }

    init_array(top_users, INT_MIN);
    init_array(worst_users, INT_MAX);
    init_array(top_nodes, INT_MIN);
    alarm(config.SO_SIM_SEC);
    unlock_init_semaphore(id_sem_init);
    while (executing && !ledger_full && active_users > 0) {
        if (msgrcv(id_msg_node_master, &node_master_msg, sizeof(node_master_message), getpid(), IPC_NOWAIT) != -1) {
            switch (node_pid = fork()) {
                case -1:
                    EXIT_ON_ERROR

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

            /*master_node_fl_msg.mtype = node_list[node_i];
            master_node_fl_msg.friends = get_random_friends(tmp_node_list[node_i]);
            msgsnd(id_msg_master_node_friend_list, &master_node_fl_msg, sizeof(master_node_fl_message), 0);
            node_master_msg.mytype = node_pid;
            msgsnd(id_msg_node_master, &node_master_msg, sizeof(node_master_message), 0);
            node_i++;

            for (i = 0; i < config.SO_NODES_NUM; i++) {
                master_node_new_friend_msg.mtype = node_list[i];
                master_node_new_friend_msg.new_friend = node_list[i];
                msgsnd(id_msg_master_node_new_friend, &master_node_new_friend_msg,
                       sizeof(master_node_new_friend_message), 0);
            }*/
        }

        lock(id_sem_writers_block_id);
        tmp_block_id = *block_id;
        unlock(id_sem_writers_block_id);
        tmp_block_id--;
        for (i = 0; i < config.SO_NODES_NUM; i++) {
            node_balance = calculate_node_balance(node_list[i], tmp_block_id);
            add_max(top_nodes, node_list[i], node_balance);
        }

        for (i = 0; i < config.SO_USERS_NUM; i++) {
            user_balance = calculate_user_balance(user_list[i], tmp_block_id);
            add_max(top_users, user_list[i], user_balance);
            add_min(worst_users, user_list[i], user_balance);
        }

        /*print_live_info(top_nodes, top_users, worst_users);*/

        nanosleep(&request, &remaining);
    }

    kill(0, SIGTERM);

    print_ledger();

    /*printf("\n%s\n", ANSI_COLOR_GREEN "=============NODES=============" ANSI_COLOR_RESET);
    printf("%8s        %5s    %8s\n", "PID", ANSI_COLOR_MAGENTA "|" ANSI_COLOR_RESET, "BALANCE");
    printf("%s\n", ANSI_COLOR_GREEN "===============================" ANSI_COLOR_RESET);
    for (i = 0; i < config.SO_NODES_NUM; i++) {
        node_balance = calculate_node_balance(node_list[i], *block_id);
        printf("%8d    %5s    %8d\n", node_list[i], "|", node_balance);
    }

    printf("\n%s\n", ANSI_COLOR_GREEN "=============USERS=============" ANSI_COLOR_RESET);
    printf("%8s        %5s    %8s\n", "PID", ANSI_COLOR_MAGENTA "|" ANSI_COLOR_RESET, "BALANCE");
    printf("%s\n", ANSI_COLOR_GREEN "===============================" ANSI_COLOR_RESET);
    for (i = 0; i < config.SO_USERS_NUM; i++) {
        user_balance = calculate_user_balance(user_list[i], *block_id);
        printf("%8d    %5s    %8d\n", user_list[i], "|", user_balance);
    }*/

    print_final_report();

    cleanIPC();

    return 0;
}

void shuffle(int *array) {
    size_t i;
    size_t j;
    int tmp;
    struct timespec tp;

    clock_gettime(CLOCK_REALTIME, &tp);
    srand(tp.tv_nsec);
    if (config.SO_NODES_NUM > 1) {
        for (i = 0; i < config.SO_NODES_NUM - 1; i++) {
            j = i + rand() / (RAND_MAX / (config.SO_NODES_NUM - i) + 1);
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

    for (i = 0; i < config.SO_NODES_NUM && added < config.SO_FRIENDS_NUM; i++) {
        if (tmp_node_list[i] != node) {
            friend_list[added++] = tmp_node_list[i];
        }
    }

    return friend_list;
}

void print_live_info(child_data *top_nodes, child_data *top_users, child_data *worst_users) {
    int i;
    printf("\n%10s        %10s        %10s        %10s         %10s\n",
           ANSI_COLOR_GREEN "==========TOP NODES==========" ANSI_COLOR_RESET,
           ANSI_COLOR_GREEN "==========TOP USERS==========" ANSI_COLOR_RESET,
           ANSI_COLOR_GREEN "=========WORST USERS=========" ANSI_COLOR_RESET,
           ANSI_COLOR_GREEN "==================" ANSI_COLOR_RESET,
           ANSI_COLOR_GREEN "==================" ANSI_COLOR_RESET);
    fflush(stdout);

    printf("%8s      %5s    %8s          %8s      %5s    %8s         %8s     %5s    %8s               %10s               %10s\n",
           "PID", ANSI_COLOR_MAGENTA "|" ANSI_COLOR_RESET, "BALANCE",
           "PID", ANSI_COLOR_MAGENTA "|" ANSI_COLOR_RESET, "BALANCE",
           "PID", ANSI_COLOR_MAGENTA "|" ANSI_COLOR_RESET, "BALANCE",
           "ACTIVE NODES",
           "ACTIVE USERS");
    fflush(stdout);
    printf(ANSI_COLOR_GREEN "=============================        =============================        =============================        ==================         ==================\n" ANSI_COLOR_RESET);
    fflush(stdout);
    for (i = 0; i < N_USER_TO_DISPLAY; i++) {
        if (i == 0) {
            printf("%8d      %5s  %8d            %8d      %5s  %8d            %8d    %5s  %8d             %10d                   %10d\n",
                   top_nodes[i].pid, ANSI_COLOR_MAGENTA "|" ANSI_COLOR_RESET, top_nodes[i].balance,
                   top_users[i].pid, ANSI_COLOR_MAGENTA "|" ANSI_COLOR_RESET, top_users[i].balance,
                   worst_users[i].pid, ANSI_COLOR_MAGENTA "|" ANSI_COLOR_RESET, worst_users[i].balance,
                   active_nodes,
                   active_users
            );
            fflush(stdout);
        } else {
            printf("%8d      %5s  %8d            %8d      %5s  %8d            %8d    %5s  %8d             %10s                   %10s\n",
                   top_nodes[i].pid, ANSI_COLOR_MAGENTA "|" ANSI_COLOR_RESET, top_nodes[i].balance,
                   top_users[i].pid, ANSI_COLOR_MAGENTA "|" ANSI_COLOR_RESET, top_users[i].balance,
                   worst_users[i].pid, ANSI_COLOR_MAGENTA "|" ANSI_COLOR_RESET, worst_users[i].balance,
                   "",
                   ""
            );
            fflush(stdout);
        }
    }
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

void add_max(child_data *array, pid_t pid, int balance) {
    int i;
    int added = 0;
    int present = 0;
    for (i = 0; i < N_USER_TO_DISPLAY && !added; i++) {
        if (pid == array[i].pid) {
            if (balance > array[i].balance) {
                array[i].balance = balance;
                qsort(array, N_USER_TO_DISPLAY, sizeof(child_data), compare);
                added = 1;
            }
            present = 1;
        }
        if (present == 0 && balance > array[i].balance) {
            memmove(array + i + 1, array + i, (N_USER_TO_DISPLAY - i - 1) * sizeof(*array));
            array[i].pid = pid;
            array[i].balance = balance;
            added = 1;
        }
    }
}

void add_min(child_data *array, pid_t pid, int balance) {
    int i;
    int added = 0;
    int present = 0;
    for (i = 0; i < N_USER_TO_DISPLAY && !added; i++) {
        if (pid == array[i].pid) {
            if (balance < array[i].balance) {
                array[i].balance = balance;
                qsort(array, N_USER_TO_DISPLAY, sizeof(child_data), compare);
                added = 1;
            }
            present = 1;
        }
        if (present == 0 && balance < array[i].balance) {
            memmove(array + i + 1, array + i, (N_USER_TO_DISPLAY - i - 1) * sizeof(*array));
            array[i].pid = pid;
            array[i].balance = balance;
            added = 1;
        }
    }
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

void init_array(child_data *array, int value) {
    int i;
    for (i = 0; i < N_USER_TO_DISPLAY; i++) {
        array[i].pid = 0;
        array[i].balance = value;
    }
}

void print_ledger() {
    int i;
    int tmp_block_id;
    lock(id_sem_writers_block_id);
    tmp_block_id = *block_id;
    unlock(id_sem_writers_block_id);
    for (i = 0; i < tmp_block_id; i++) {
        print_block(master_ledger->blocks[i]);
        printf(ANSI_COLOR_GREEN "=============================================================================================================================\n" ANSI_COLOR_RESET);
    }
}

void print_final_report() {
    printf(ANSI_COLOR_CYAN "\n\n==================SIMULATION REPORT==================\n" ANSI_COLOR_RESET);
    printf("%s %s", ANSI_COLOR_CYAN "|" ANSI_COLOR_RESET, "Reason of ending: ");
    if (executing == 0) {
        printf("%s", "time expired");
    }
    if (ledger_full == 1) {
        printf("%s", ", ledger full");
    }
    if (active_users == 0) {
        printf("%s", ", all users are dead");
    }
    printf(ANSI_COLOR_CYAN "%s\n", "                    |" ANSI_COLOR_RESET);
    printf("%s", ANSI_COLOR_CYAN "|" ANSI_COLOR_RESET);
    printf(" User processes dead = %d %s\n", config.SO_USERS_NUM - final_alive_users,
           ANSI_COLOR_CYAN "                          |" ANSI_COLOR_RESET);
    printf("%s", ANSI_COLOR_CYAN "|" ANSI_COLOR_RESET);
    printf(" Number of blocks in the ledger = %d %s\n", *block_id, ANSI_COLOR_CYAN "             |" ANSI_COLOR_RESET);
    printf(ANSI_COLOR_CYAN "================SIMULATION TERMINATED================\n" ANSI_COLOR_RESET);
}

void read_configuration(configuration *config) {
    FILE *file;
    char s[23];
    char comment;
    char filename[] = "../configurations/configuration1.conf";
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
        EXIT_ON_ERROR
    }

    if (i < 10) {
        printf("Configuration not valid\n");
        kill(0, SIGINT);
    }

    fclose(file);
}

void handler(int signal) {
    union semun sem_ds;
    int sem_value;
    sem_ds.val = 0;
    switch (signal) {
        case SIGALRM:
            final_alive_users = active_users;
            sem_value = semctl(id_sem_writers_block_id, 0, GETVAL, sem_ds.val);
            if (sem_value == 0) {
                unlock(id_sem_writers_block_id);
            }
            executing = 0;
            break;

        case SIGINT:
            /*sem_value = semctl(id_sem_writers_block_id, 0, GETVAL, sem_ds.val);
            if (sem_value == 0) {
                unlock(id_sem_writers_block_id);
            }
            executing = 0;*/
            break;

        case SIGTERM:
            executing = 0;
            break;

        case SIGQUIT:
            /*sem_value = semctl(id_sem_writers_block_id, 0, GETVAL, sem_ds.val);
            if (sem_value == 0) {
                unlock(id_sem_writers_block_id);
            }
            executing = 0;*/
            break;

        case SIGUSR1:
            /*sem_value = semctl(id_sem_writers_block_id, 0, GETVAL, sem_ds.val);
            if (sem_value == 0) {
                unlock(id_sem_writers_block_id);
            }*/
            active_users--;
            break;

        case SIGUSR2:
            active_nodes--;
            break;

        default:
            break;
    }
}

void cleanIPC() {
    free(node_list);

    shmdt(master_ledger);
    shmdt(user_list);
    shmdt(block_id);
    shmdt(readers_block_id);

    shmctl(id_shm_ledger, IPC_RMID, NULL);
    shmctl(id_shm_user_list, IPC_RMID, NULL);
    shmctl(id_shm_block_id, IPC_RMID, NULL);
    shmctl(id_shm_readers_block_id, IPC_RMID, NULL);

    msgctl(id_msg_node_user, IPC_RMID, NULL);
    msgctl(id_msg_user_node, IPC_RMID, NULL);

    semctl(id_sem_init, 0, IPC_RMID);
    semctl(id_sem_writers_block_id, 0, IPC_RMID);
    semctl(id_sem_readers_block_id, 0, IPC_RMID);
}