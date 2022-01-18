#include "util.h"
#include "semlib.h"

#define SENDER_TRANSACTION_REWARD -1
#define TIMER_NEW_FRIEND_TRANSACTION 4

typedef struct {
    transaction *transactions;
} transaction_pool;

int add_to_transaction_pool(transaction);

int remove_from_transaction_pool(transaction);

transaction *extract_transactions_block_from_pool();

block new_block(transaction []);

transaction new_reward_transaction(int);

int add_to_ledger(block block);

void print_transaction_pool();

void update_info();

void clean_transaction_pool();

void add_new_friend(pid_t node);

pid_t get_random_friend();

void handler(int);