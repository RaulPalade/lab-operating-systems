#ifndef __SIGLIB_H
#define __SIGLIB_H

void mask(int, sigset_t);

void unmask(int, sigset_t);

#endif