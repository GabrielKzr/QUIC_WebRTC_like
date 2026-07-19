#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 199309L
#endif
#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>

#include <time.h>

extern int debug;

static inline void debug_print_ts(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    fprintf(stderr, "[%ld.%03ld] ", (long)ts.tv_sec, ts.tv_nsec / 1000000);
}

#ifdef NDEBUG
#define DEBUG_PRINT(fmt, ...) \
    do { debug_print_ts(); printf(fmt, ##__VA_ARGS__); } while (0)
#else
#define DEBUG_PRINT(fmt, ...) \
    do { if (debug) { debug_print_ts(); printf(fmt, ##__VA_ARGS__); } } while(0)
#endif /* NDEBUG */

#define max(a,b)             \
({                           \
    __typeof__ (a) _a = (a); \
    __typeof__ (b) _b = (b); \
    _a > _b ? _a : _b;       \
})

void usage(int argc, char *argv[], int *DEBUG, char **mode, int *localport, 
           char **remoteaddr, int *remoteport);

void print_sockaddr_in(const struct sockaddr_in *addr);
void msleep(int ms);

#endif