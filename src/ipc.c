#include "ipc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

int init_ipc_server(int port) {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        return -1;
    }
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("bind");
        close(server_fd);
        return -1;
    }
    if (listen(server_fd, 5) < 0) {
        perror("listen");
        close(server_fd);
        return -1;
    }
    return server_fd;
}

int init_ipc_client(const char *server_ip, int port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        return -1;
    }
    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, server_ip, &serv_addr.sin_addr) <= 0) {
        perror("inet_pton");
        close(sock);
        return -1;
    }
    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("connect");
        close(sock);
        return -1;
    }
    return sock;
}

void send_message(int socket, const char *message) {
    send(socket, message, strlen(message), 0);
}

char* receive_message(int socket) {
    char buffer[1024];
    int valread = recv(socket, buffer, 1024, 0);
    if (valread > 0) {
        buffer[valread] = '\0';
        return strdup(buffer);
    }
    return NULL;
}

void close_ipc_connection(int socket) {
    close(socket);
}