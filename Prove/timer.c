#include "../util.h"

int executing = 1;
void handler(int);

int main() {
    int i = 1;
    struct sigaction sa;
    struct timespec interval;
    interval.tv_sec = 1;
    interval.tv_nsec = 0;
    
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handler;
    sigaction(SIGALRM, &sa, 0);

    alarm(5);

    while (executing){
        printf("%d\n", i);
        i++;
        nanosleep(&interval, NULL);
    }

    return 0;
}

void handler(int signal) {
    if (signal == SIGALRM) {
        executing = 0;
    }
}