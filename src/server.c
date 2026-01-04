#include "server.h"
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

// Global game session
GameSession session;

void process_message(int client_index, const char *message) {
    Client *client = &session.clients[client_index];
    if (strcmp(message, MSG_JOIN) == 0) {
        if (client->snake_index == -1 && session.game.num_snakes < session.max_clients) {
            add_snake(&session.game, rand() % session.game.width, rand() % session.game.height);
            client->snake_index = session.game.num_snakes - 1;
            send_message(client->client_socket, RSP_OK);
        } else {
            send_message(client->client_socket, RSP_FULL);
        }
    } else if (strcmp(message, MSG_LEAVE) == 0) {
        if (client->snake_index != -1) {
            remove_snake(&session.game, client->snake_index);
            client->snake_index = -1;
        }
        send_message(client->client_socket, RSP_OK);
    } else if (client->snake_index != -1) {
        Snake *snake = &session.game.snakes[client->snake_index];
        if (strcmp(message, MSG_MOVE_UP) == 0) {
            change_direction(snake, 0);
        } else if (strcmp(message, MSG_MOVE_DOWN) == 0) {
            change_direction(snake, 2);
        } else if (strcmp(message, MSG_MOVE_LEFT) == 0) {
            change_direction(snake, 3);
        } else if (strcmp(message, MSG_MOVE_RIGHT) == 0) {
            change_direction(snake, 1);
        }
        send_message(client->client_socket, RSP_OK);
    } else {
        send_message(client->client_socket, RSP_ERROR);
    }
}

DWORD WINAPI handle_client(LPVOID lpParam) {
    int client_socket = (int)lpParam;
    int client_index = -1;
    for (int i = 0; i < session.num_clients; i++) {
        if (session.clients[i].client_socket == client_socket) {
            client_index = i;
            break;
        }
    }
    if (client_index == -1) return 0;

    while (1) {
        char *msg = receive_message(client_socket);
        if (msg == NULL) break;
        process_message(client_index, msg);
        free(msg);
    }
    // Cleanup
    if (session.clients[client_index].snake_index != -1) {
        remove_snake(&session.game, session.clients[client_index].snake_index);
    }
    close_ipc_connection(client_socket);
    return 0;
}

DWORD WINAPI game_update_thread(LPVOID lpParam) {
    while (1) {
        Sleep(500); // Update every 500ms
        update_game(&session.game);
        // TODO: Send game state to clients
        for (int i = 0; i < session.num_clients; i++) {
            if (session.clients[i].snake_index != -1) {
                // Send game state
                send_message(session.clients[i].client_socket, MSG_GAME_STATE);
                // TODO: Serialize game state
            }
        }
    }
    return 0;
}

void run_server(int port) {
    // Initialize session
    session.num_clients = 0;
    session.max_clients = 10;
    session.clients = malloc(session.max_clients * sizeof(Client));
    init_game(&session.game, 20, 20, false, 0, 0); // Default game
    generate_obstacles(&session.game);

    // Start game update thread
    HANDLE update_thread = CreateThread(NULL, 0, game_update_thread, NULL, 0, NULL);
    if (update_thread) CloseHandle(update_thread);

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