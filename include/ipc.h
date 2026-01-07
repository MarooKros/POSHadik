#ifndef IPC_H
#define IPC_H

#include <stdbool.h>

typedef struct {
    int socket_fd;
} IPCConnection;

int init_ipc_server(int port); // returns server socket fd or -1 on error
int init_ipc_client(const char *server_ip, int port); // returns client socket fd or -1 on error
void send_message(int socket, const char *message);
char* receive_message(int socket);
void close_ipc_connection(int socket);

#endif 