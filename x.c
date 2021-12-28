#include "util.h"

configuration (*config);

node_information (*node_info);
user_information (*user_info);

int id_shared_memory_configuration;
int id_shared_memory_nodes;
int id_shared_memory_users;

int main() {
    int i;
    key_t key;

    if ((key = ftok("./makefile", 'x')) < 0) {
        raise(SIGQUIT);
    }

    if ((id_shared_memory_configuration = shmget(key, sizeof(configuration), IPC_CREAT | 0666)) < 0) {
        raise(SIGQUIT);
    }

    if ((void *) (config = shmat(id_shared_memory_configuration, NULL, 0)) < (void *) 0) {
        raise(SIGQUIT);
    }

    read_configuration(config);

    /* NODES AND USERS SHARED MEMORY TO STORE ID AND BALANCE */
    if ((key = ftok("./makefile", 'w')) < 0) {
        raise(SIGQUIT);
    }

    if ((id_shared_memory_nodes = shmget(key, config->SO_NODES_NUM, IPC_CREAT | 0666)) < 0) {
        raise(SIGQUIT);
    }

    if ((void *) (node_info = shmat(id_shared_memory_nodes, NULL, 0)) < (void *) 0) {
        raise(SIGQUIT);
    }

    if ((key = ftok("./makefile", 'z')) < 0) {
        raise(SIGQUIT);
    }

    if ((id_shared_memory_users = shmget(key, config->SO_USERS_NUM, IPC_CREAT | 0666)) < 0) {
        raise(SIGQUIT);
    }

    if ((void *) (user_info = shmat(id_shared_memory_users, NULL, 0)) < (void *) 0) {
        raise(SIGQUIT);
    }

    printf("id_shared_memory_configuration=%d\n", id_shared_memory_configuration);
    printf("id_shared_memory_nodes=%d\n", id_shared_memory_nodes);
    printf("id_shared_memory_users=%d\n", id_shared_memory_users);

    printf("Launching Node processes\n");
    for (i = 0; i < (*config).SO_NODES_NUM; i++) {
        switch (fork())
        {
        case -1:
            EXIT_ON_ERROR
        
        case 0:
            node_info[i].pid = getpid();
            node_info[i].balance = 0;
            node_info[i].transactions_left = 0;
            /* execute_node(i); */
        }
    }

    printf("Launching User processes\n");
    for (i = 0; i < (*config).SO_USERS_NUM; i++) {
        switch (fork())
        {
        case -1:
            EXIT_ON_ERROR
        
        case 0:
            user_info[i].pid = getpid();
            user_info[i].balance = 0;
            /* execute_user(i); */
        }
    }

    for (i = 0; i < config->SO_NODES_NUM; i++) {
        printf("Node PID=%d    Balance=%d      TransactionsLeft=%d\n", node_info[i].pid, node_info[i].balance, node_info[i].transactions_left);
    }

    for (i = 0; i < config->SO_USERS_NUM; i++) {
        printf("User PID=%d    Balance=%d\n", user_info[i].pid, user_info[i].balance);
    }

    return 0;
}