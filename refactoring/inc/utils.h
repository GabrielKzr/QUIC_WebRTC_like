#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>

extern int debug;

#define DEBUG_PRINT(fmt, ...) \
    do { if (debug) printf(fmt, ##__VA_ARGS__); } while(0)

#define max(a,b)             \
({                           \
    __typeof__ (a) _a = (a); \
    __typeof__ (b) _b = (b); \
    _a > _b ? _a : _b;       \
})

void usage(int argc, char *argv[], int *DEBUG, char **mode, int *localport, 
           char **remoteaddr, int *remoteport);

void print_sockaddr_in(const struct sockaddr_in *addr);

#endif