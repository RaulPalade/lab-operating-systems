# Lab Operating Systems

The aim is to simulate a ledger containing data of monetary transactions between different users. To this end, the following processes are present:
- a master process that manages the simulation, the creation of other processes, etc.
- `SO_USERS_NUM` user processes that can send money to other users through a transaction
- `SO_NODES_NUM` node processes that process, for a fee, the received transactions.

## Transactions

A transaction is characterized by the following information:
- timestamp of the transaction with nanosecond resolution (see `clock_gettime(...)` function)
- sender (implicit, as it is the user who generated the transaction)
- receiver, the recipient of the sum
- amount of money sent
- reward, money paid by the sender to the node that processes the transaction
The transaction is sent by the user process that generates it to one of the node processes, chosen randomly.

##  User Processes

The user processes are responsible for creating and sending monetary transactions to the node processes. Each user process is assigned an initial budget of `SO_BUDGET_INIT`. During its lifetime, a user process repeatedly performs the following operations:

Calculates the current balance starting from the initial budget and performing the algebraic sum of the income and expenses recorded in the transactions present in the ledger, subtracting the amounts of the transactions sent but not yet recorded in the ledger.
1. If the balance is greater than or equal to 2, the process randomly extracts:
   - Another user process recipient to send money to
   - A node to send the transaction to be processed
   - An integer value between 2 and its balance divided in this way:
     - the reward of the transaction is equal to a `SO_REWARD` percentage of the extracted value, rounded, with a minimum of 1,
     - the amount of the transaction will be equal to the extracted value subtracted from the reward
     
     Example: the user has a balance of 100. By randomly extracting a number between 2 and 100, it extracts 50.
     
     If `SO_REWARD` is 20 (indicating a 20% reward), then with the execution of the transaction the user will transfer 40 to the recipient user and 10 to the node that will have successfully processed the transaction.

   - If the balance is less than 2, then the process does not send any transaction

2. Sends the extracted transaction to the node and waits for a randomly extracted time interval (in nanoseconds) between `SO_MIN_TRANS_GEN_NSEC` and maximum `SO_MAX_TRANS_GEN_NSEC`.

In addition, a user process must also generate a transaction in response to a received signal (the choice of the signal is at the discretion of the developers).
If a process fails to send any transaction for `SO_RETRY` consecutive times, then it terminates its execution, notifying the master process.

## Node Process
Each node process privately stores a list of transactions received for processing, called the transaction pool, which can contain a maximum of `SO_TP_SIZE` transactions, with `SO_TP_SIZE > SO_BLOCK_SIZE`. If the node's transaction pool is full, then any further transactions are discarded and not executed. In this case, the sender of the discarded transaction must be informed.
Transactions are processed by a node in blocks. Each block contains exactly `SO_BLOCK_SIZE` transactions to be processed, of which `SO_BLOCK_SIZE − 1` are transactions received from users and one is a payment transaction for processing (see below).
The life cycle of a node can be defined as follows:
- Creation of a candidate block
  - Extracting from the transaction pool a set of `SO_BLOCK_SIZE − 1` transactions not yet present in the ledger
  - To the transactions present in the block, the node adds a reward transaction, with the following characteristics:
    - *timestamp*: the current value of `clock_gettime(...)`
    - *sender*: -1 (define a MACRO...)
    - *receiver*: the current node's identifier
    - *amount*: the sum of all rewards of the transactions included in the block
    - *reward*: 0
- Simulates the processing of a block by waiting passively for a random time interval in nanoseconds between `SO_MIN_TRANS_PROC_NSEC` and `SO_MAX_TRANS_PROC_NSEC`.
- Once the block processing is completed, it writes the newly processed block to the ledger, and removes the successfully executed transactions from the transaction pool.

When created by the master process, each node receives a list of `SO_NUM_FRIENDS` friend nodes.
The life cycle of a node process is therefore enriched with an additional step:
- periodically, each node selects a transaction from the transaction pool that is not yet in the ledger and sends it to a randomly chosen friend node (the transaction is removed from the source node's transaction pool)

When a node receives a transaction, but has a full transaction pool, it will send the transaction to one of its friends chosen at random. If the transaction cannot find a location within `SO_HOPS`, the last node that receives it will send the transaction to the master process, which will create a new node process that contains the discarded transaction as the first element of the transaction pool. In addition, the master process assigns `SO_NUM_FRIENDS` randomly chosen existing node processes as friends to the new node process. Furthermore, the master process will randomly choose other `SO_NUM_FRIENDS` existing node processes and order them to add the newly created node process to their list of friends.

## Ledger

The ledger is the structure shared by all nodes and users, and is responsible for storing executed transactions. A transaction is considered confirmed only when it is part of the ledger. In more detail, the ledger is made up of a maximum length `SO_REGISTRY_SIZE` sequence of consecutive blocks. Each block contains exactly `SO_BLOCK_SIZE` transactions. Each block is identified by a progressive integer identifier whose initial value is set to 0.

A transaction is uniquely identified by the triplet (*timestamp, sender, receiver*). The node that adds a new block to the ledger is also responsible for updating the block's identifier.

## Print
Each second, the master process prints:
- number of active user processes to node
- the current budget of each user process and each node process, as recorded in the ledger (including terminated user processes). If the number of processes is too large to be displayed, then only the state of the most significant processes is printed: those with the highest and lowest budget.

## End of the simulation
The simulation will end in one of the following cases:
- `SO_SIM_SEC` seconds have passed,
- the ledger capacity is exhausted (the ledger can contain a maximum of `SO_REGISTRY_SIZE` blocks),
- all user processes have terminated.

Upon termination, the master process forces all node processes and user processes to terminate, and prints a summary of the simulation, containing at least the following information:

- reason for the simulation termination
- balance of each user process, including those that prematurely terminated
- balance of each node process
• number of user processes that prematurely terminated
• number of blocks in the ledger
• for each node process, the number of transactions still present in the transaction pool.

## Configuration
The following parameters are read at runtime, from a file, from environment variables, or from stdin (at the discretion of the students):
- `SO_USERS_NUM`: number of user processes
- `SO_NODES_NUM`: number of node processes
- `SO_BUDGET_INIT`: initial budget of each user process
- `SO_REWARD`: the percentage of reward paid by each user for the processing of a transaction
- `SO_MIN_TRANS_GEN_NSEC`, `SO_MAX_TRANS_GEN_NSEC`: minimum and maximum value of the time (expressed in nanoseconds) that elapses between the generation of a transaction and the next one by a user
- `SO_RETRY`, maximum number of consecutive failures in transaction generation after which a user process terminates
- `SO_TP_SIZE`: maximum number of transactions in the transaction pool of the node processes
- `SO_MIN_TRANS_PROC_NSEC`, `SO_MAX_TRANS_PROC_NSEC`: minimum and maximum value of the simulated time (expressed in nanoseconds) for processing a block by a node
- `SO_SIM_SEC`: duration of the simulation (in seconds)
- `SO_NUM_FRIENDS`: number of friend nodes of the node processes
- `SO_HOPS`: maximum number of forwarding of a transaction to friend nodes before the master creates a new node.

A change in the previous parameters should not result in a new compilation of the sources.
Instead, the following parameters are read at compile time:
- `SO_REGISTRY_SIZE`: maximum number of blocks in the ledger
- `SO_BLOCK_SIZE`: number of transactions contained in a block
The following table lists values for some sample configurations to test. Keep in mind that the project must also be able to work with other parameters.

| Parameter | Read at | Conf 1 | Conf 2 | Conf 3 |
| :---      |  :---:  |  ---:  |  ---: |  ---:  |
| `SO_USERS_NUM` | run time | 100 | 1000 | 20 |
| `SO_NODES_NUM` | run time | 10 | 10 | 10 |
| `SO_BUDGET_INIT` | run time | 1000 | 1000 | 10000 |
| `SO_REWARD [0–100]` | run time | 1 | 20 | 1 |
| `SO_MIN_TRANS_GEN_NSEC [nsec]` | run time | 100000000 | 100000000 | 100000000 |
| `SO_MAX_TRANS_GEN_NSEC [nsec]` | run time | 200000000 | 100000000 | 200000000 |
| `SO_RETRY` | run time | 20 | 2 | 10 |
| `SO_TP_SIZE` | run time | 1000 | 20 | 100 |
| `SO_BLOCK_SIZE` | compile time | 100 | 10 | 10 |
| `SO_MIN_TRANS_PROC_NSEC [nsec]` | run time | 10000000 | 10000000 |  |
| `SO_MAX_TRANS_PROC_NSEC [nsec]` | run time | 20000000 | 10000000 |  |
| `SO_REGISTRY_SIZE` | compile time | 1000 | 10000 | 1000 |
| `SO_SIM_SEC [sec]` | run time | 10 | 20 | 20 |
| `SO_FRIENDS_NUM` | run time | 3 | 5 | 3 |
| `SO_HOPS` | run time | 10 | 2 | 10 |

## Requisiti implementativi
The project must:
- be implemented using code modularization techniques,
- be compiled using the make utility,
- maximize the degree of concurrency among processes,
- deallocate IPC resources that have been allocated by processes at the end of the game,
- be compiled with at least the following compilation options: `gcc -std=c89 -pedantic`

- be able to run correctly on a machine (virtual or physical) that has parallelism (two or more processors).
For the reasons introduced in class, remember to define the `_GNU_SOURCE` macro or compile the project with the `-D_GNU_SOURCE` flag.
