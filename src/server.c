#include "server.h"
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

// Thread function for handling client
DWORD WINAPI handle_client(LPVOID lpParam) {
    int client_socket = (int)lpParam;
    // TODO: Handle client communication
    close_ipc_connection(client_socket);
    return 0;
}

void run_server(int port) {
    int server_fd = init_ipc_server(port);
    if (server_fd == -1) {
        fprintf(stderr, "Failed to initialize server\n");
        return;
    }
    printf("Server started on port %d\n", port);
    // TODO: Accept connections, handle game logic in threads
    while (1) {
        // Accept client
        struct sockaddr_in client_addr;
        int addr_len = sizeof(client_addr);
        int client_socket = accept(server_fd, (struct sockaddr*)&client_addr, &addr_len);
        if (client_socket == INVALID_SOCKET) {
            perror("accept");
            continue;
        }
        // Create thread for client
        HANDLE thread = CreateThread(NULL, 0, handle_client, (LPVOID)client_socket, 0, NULL);
        if (thread == NULL) {
            perror("CreateThread");
            close_ipc_connection(client_socket);
        } else {
            CloseHandle(thread); // Detach thread
        }
    }
    close_ipc_connection(server_fd);
}