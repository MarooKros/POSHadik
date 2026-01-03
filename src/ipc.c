#include "ipc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

int init_ipc_server(int port) {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        fprintf(stderr, "WSAStartup failed\n");
        return -1;
    }
    SOCKET server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == INVALID_SOCKET) {
        perror("socket");
        WSACleanup();
        return -1;
    }
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) == SOCKET_ERROR) {
        perror("bind");
        closesocket(server_fd);
        WSACleanup();
        return -1;
    }
    if (listen(server_fd, 5) == SOCKET_ERROR) {
        perror("listen");
        closesocket(server_fd);
        WSACleanup();
        return -1;
    }
    return (int)server_fd;
}

int init_ipc_client(const char *server_ip, int port) {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        fprintf(stderr, "WSAStartup failed\n");
        return -1;
    }
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        perror("socket");
        WSACleanup();
        return -1;
    }
    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, server_ip, &serv_addr.sin_addr) <= 0) {
        perror("inet_pton");
        closesocket(sock);
        WSACleanup();
        return -1;
    }
    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == SOCKET_ERROR) {
        perror("connect");
        closesocket(sock);
        WSACleanup();
        return -1;
    }
    return (int)sock;
}

void send_message(int socket, const char *message) {
    send(socket, message, strlen(message), 0);
}

char* receive_message(int socket) {
    char buffer[1024];
    int valread = recv(socket, buffer, 1024, 0);
    if (valread > 0) {
        buffer[valread] = '\0';
        return _strdup(buffer);
    }
    return NULL;
}

void close_ipc_connection(int socket) {
    closesocket(socket);
    WSACleanup();
}