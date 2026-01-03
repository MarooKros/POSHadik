#include "server.h"
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

// Global game session
GameSession session;

DWORD WINAPI handle_client(LPVOID lpParam) {
    int client_socket = (int)lpParam;
    // TODO: Handle client messages, add to game
    char *msg = receive_message(client_socket);
    if (msg) {
        printf("Received: %s\n", msg);
        free(msg);
    }
    close_ipc_connection(client_socket);
    return 0;
}

void run_server(int port) {
    // Initialize session
    session.num_clients = 0;
    session.max_clients = 10;
    session.clients = malloc(session.max_clients * sizeof(Client));
    init_game(&session.game, 20, 20, false, 0, 0); // Default game

    int server_fd = init_ipc_server(port);
    if (server_fd == -1) {
        fprintf(stderr, "Failed to initialize server\n");
        return;
    }
    printf("Server started on port %d\n", port);
    while (1) {
        struct sockaddr_in client_addr;
        int addr_len = sizeof(client_addr);
        int client_socket = accept(server_fd, (struct sockaddr*)&client_addr, &addr_len);
        if (client_socket == INVALID_SOCKET) {
            perror("accept");
            continue;
        }
        // Add client
        if (session.num_clients < session.max_clients) {
            session.clients[session.num_clients].client_socket = client_socket;
            session.clients[session.num_clients].snake_index = -1;
            session.num_clients++;
            HANDLE thread = CreateThread(NULL, 0, handle_client, (LPVOID)client_socket, 0, NULL);
            if (thread) CloseHandle(thread);
        } else {
            close_ipc_connection(client_socket);
        }
    }
    close_ipc_connection(server_fd);
    free(session.clients);
}