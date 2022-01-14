#include "../include/util.h"
#include "../include/siglib.h"

void mask(int signal, sigset_t my_mask) {
    sigemptyset(&my_mask);
    sigaddset(&my_mask, signal);
    sigprocmask(SIG_BLOCK, &my_mask, NULL);
}

void unmask(int signal, sigset_t my_mask) {
    sigaddset(&my_mask, signal);
    sigprocmask(SIG_UNBLOCK, &my_mask, NULL);
}