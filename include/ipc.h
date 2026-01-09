#ifndef IPC_H
#define IPC_H

int init_ipc_server(int port); 
int init_ipc_client(const char *server_ip, int port); 
void send_message(int socket, const char *message);
char* receive_message(int socket);
void close_ipc_connection(int socket);

#endif 