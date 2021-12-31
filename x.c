#include "util.h"

configuration (*config);
ledger (*master_ledger);

node_information (*node_info);
user_information (*user_info);

int *last_block_id;     /* Used to read by user till this block */

int id_shared_memory_ledger;            /* Ledger */
int id_shared_memory_configuration;     /* Configuration */
int id_shared_memory_last_block_id;          /* Used to keep trace of block_id for next block and to read operations */
int id_shared_memory_nodes;
int id_shared_memory_users;

int id_message_queue_master_node;
int id_message_queue_master_user;
int id_message_queue_node_user;

int id_semaphore_init;
int id_semaphore_writers;
int id_semaphore_mutex;

volatile int executing = 1;
int ledger_full = 0;
int dead_users = 0;

static int active_nodes = 0;
static int active_users = 0;

typedef struct {
    long mtype;
    char letter;
} chat_message;


int main() {
    chat_message cm;
    struct msqid_ds buf;
    configuration c;
    int remaining_seconds;
    pid_t node_pid;
    pid_t user_pid;
    int i;
    key_t key;
    struct sigaction sa;
    union semun sem_arg;
    struct semid_ds writers_ds;
    struct timespec interval;
    interval.tv_sec = 1;
    interval.tv_nsec = 0;

    if ((key = ftok("./makefile", 'a') ) < 0) {
        EXIT_ON_ERROR
        raise(SIGQUIT);
    } else {
        printf("key = %d\n", key);
    }

    if ((id_message_queue_node_user = msgget(key, IPC_CREAT | 0666)) < 0) {
        EXIT_ON_ERROR
        raise(SIGQUIT);
    } else {
        printf("id_message_queue_node_user = %d\n", id_message_queue_node_user);
    }


    cm.mtype = 0;
    cm.letter = 'A';
    msgsnd(id_message_queue_node_user, &cm, sizeof(chat_message), 0);
    msgctl(id_message_queue_node_user, IPC_STAT, &buf);
    printf("Total messages in queue = %ld\n", buf.msg_qnum);

    msgctl(id_message_queue_node_user, IPC_RMID, NULL);
    return 0;
}