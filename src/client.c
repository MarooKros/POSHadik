#include "client.h"
#include <stdio.h>
#include <stdlib.h>

void run_client(const char *server_ip, int port) {
    int sock = init_ipc_client(server_ip, port);
    if (sock == -1) {
        fprintf(stderr, "Failed to connect to server\n");
        return;
    }
    printf("Connected to server %s:%d\n", server_ip, port);
    // TODO: Handle user input, send to server, receive updates
}