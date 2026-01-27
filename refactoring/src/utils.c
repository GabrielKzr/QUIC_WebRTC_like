#include <utils.h>

int debug = 0;

void usage(int argc, char *argv[], int *DEBUG, char **mode, int *localport, 
           char **remoteaddr, int *remoteport) {
    int arg_idx = 1;
    
    *DEBUG = 0;
    while (arg_idx < argc && strcmp(argv[arg_idx], "-d") == 0) {
        (*DEBUG)++;
        arg_idx++;
    }
    
    if (arg_idx < argc && strcmp(argv[arg_idx], "-dd") == 0) {
        *DEBUG = 2;
        arg_idx++;
    }
    
    if (argc - arg_idx < 3) {
        fprintf(stderr, "usage: %s [-d] <-c|-s> <local port> <dest host> [communication port]\n\n", argv[0]);
        fprintf(stderr, "  -d debug mode, two -d's for verbose debug mode\n");
        fprintf(stderr, "  -c client mode\n");
        fprintf(stderr, "  -s server mode\n");
        fprintf(stderr, "  <local port>   local port to listen on or connect to\n");
        fprintf(stderr, "  <dest host>    destination host to connect to\n");
        fprintf(stderr, "  [comm port]    port to communicate on, default of 2222\n");
        exit(1);
    }
    
    *mode = argv[arg_idx++];
    if (strcmp(*mode, "-c") != 0 && strcmp(*mode, "-s") != 0) {
        fprintf(stderr, "Error: mode must be -c or -s\n");
        exit(1);
    }
    
    *localport = atoi(argv[arg_idx++]);
    
    *remoteaddr = argv[arg_idx++];
    
    if (arg_idx < argc) {
        *remoteport = atoi(argv[arg_idx]);
    } else {
        *remoteport = 2222;
    }
    
    debug = *DEBUG;
}

void print_sockaddr_in(const struct sockaddr_in *addr) {
    char ip[INET_ADDRSTRLEN];

    inet_ntop(AF_INET, &(addr->sin_addr), ip, INET_ADDRSTRLEN);

    DEBUG_PRINT("sockaddr_in {\n");
    DEBUG_PRINT("  sin_family = %d (AF_INET = %d)\n", addr->sin_family, AF_INET);
    DEBUG_PRINT("  sin_port   = %u\n", ntohs(addr->sin_port));
    DEBUG_PRINT("  sin_addr   = %s\n", ip);
    DEBUG_PRINT("  sin_zero   = { ");

    for (int i = 0; i < 8; i++) {
        DEBUG_PRINT("0x%02X ", addr->sin_zero[i]);
    }

    DEBUG_PRINT("}\n");
    DEBUG_PRINT("}\n");
}