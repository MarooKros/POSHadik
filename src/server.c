#include "server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#endif

GameSession session;

void process_message(int client_index, const char *message) {
    Client *client = &session.clients[client_index];
    char cmd[256];
    strncpy(cmd, message, sizeof(cmd) - 1);
    cmd[sizeof(cmd) - 1] = '\0';
    char *token = strtok(cmd, " ");
    if (strcmp(token, MSG_NEW_GAME) == 0) {
        int width = 20, height = 20, mode = 0, time_limit = 0, obstacles = 0;
        token = strtok(NULL, " ");
        if (token) width = atoi(token);
        token = strtok(NULL, " ");
        if (token) height = atoi(token);
        token = strtok(NULL, " ");
        if (token) mode = atoi(token);
        token = strtok(NULL, " ");
        if (token) time_limit = atoi(token);
        token = strtok(NULL, " ");
        if (token) obstacles = atoi(token);
        for (int i = 0; i < session.num_clients; i++) {
            session.clients[i].snake_index = -1;
        }
        free(session.game.snakes);
        free(session.game.fruits);
        free(session.game.obstacles);
        init_game(&session.game, width, height, obstacles > 0, mode, time_limit);
        if (obstacles == 1) {
            generate_obstacles(&session.game);
        } else if (obstacles == 2) {
            load_obstacles_from_file(&session.game, "obstacles.txt");
        }
        add_snake(&session.game, rand() % session.game.width, rand() % session.game.height);
        client->snake_index = 0;
        send_message(client->client_socket, RSP_OK);
        return;
    } else if (strcmp(token, MSG_JOIN) == 0) {
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
    } else if (strcmp(message, MSG_PAUSE) == 0) {
        send_message(client->client_socket, RSP_OK);
    } else if (strcmp(message, MSG_RESUME) == 0) {
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

#ifdef _WIN32
DWORD WINAPI handle_client(LPVOID lpParam) {
#else
void* handle_client(void* lpParam) {
#endif
    int client_socket = (int)lpParam;
    int client_index = -1;
    for (int i = 0; i < session.num_clients; i++) {
        if (session.clients[i].client_socket == client_socket) {
            client_index = i;
            break;
        }
    }
    if (client_index == -1) 
#ifdef _WIN32
        return 0;
#else
        return NULL;
#endif

    while (1) {
        char *msg = receive_message(client_socket);
        if (msg == NULL) break;
        process_message(client_index, msg);
        free(msg);
    }
    if (session.clients[client_index].snake_index != -1) {
        remove_snake(&session.game, session.clients[client_index].snake_index);
    }
    close_ipc_connection(client_socket);
#ifdef _WIN32
    return 0;
#else
    return NULL;
#endif
}

#ifdef _WIN32
DWORD WINAPI game_update_thread(LPVOID lpParam) {
#else
void* game_update_thread(void* lpParam) {
#endif
    while (1) {
#ifdef _WIN32
        Sleep(500);
#else
        usleep(500000);
#endif
        update_game(&session.game);
        char *state = serialize_game_state(&session.game);
        for (int i = 0; i < session.num_clients; i++) {
            if (session.clients[i].snake_index != -1 || session.game.game_over) {
                send_message(session.clients[i].client_socket, MSG_GAME_STATE);
                send_message(session.clients[i].client_socket, state);
            }
        }
        free(state);
    }
#ifdef _WIN32
    return 0;
#else
    return NULL;
#endif
}

void run_server(int port) {
    session.num_clients = 0;
    session.max_clients = 10;
    session.clients = malloc(session.max_clients * sizeof(Client));
    init_game(&session.game, 20, 20, false, 0, 0);
    generate_obstacles(&session.game);
#ifdef _WIN32
    HANDLE update_thread = CreateThread(NULL, 0, game_update_thread, NULL, 0, NULL);
    if (update_thread) CloseHandle(update_thread);
#else
    pthread_t update_thread;
    pthread_create(&update_thread, NULL, game_update_thread, NULL);
#endif

    int server_fd = init_ipc_server(port);
    if (server_fd == -1) {
        fprintf(stderr, "Failed to initialize server\n");
        return;
    }
    printf("Server started on port %d\n", port);
    while (1) {
        struct sockaddr_in client_addr;
        int addr_len = sizeof(client_addr);
        printf("Waiting for connection...\n");
        int client_socket = accept(server_fd, (struct sockaddr*)&client_addr, &addr_len);
        if (client_socket < 0) {
            perror("accept");
            continue;
        }
        printf("Accepted connection\n");
        if (session.num_clients < session.max_clients) {
            session.clients[session.num_clients].client_socket = client_socket;
            session.clients[session.num_clients].snake_index = -1;
            session.num_clients++;
#ifdef _WIN32
            HANDLE thread = CreateThread(NULL, 0, handle_client, (LPVOID)client_socket, 0, NULL);
            if (thread) CloseHandle(thread);
#else
            pthread_t thread;
            pthread_create(&thread, NULL, handle_client, (void*)client_socket);
#endif
        } else {
            close_ipc_connection(client_socket);
        }
    }
    close_ipc_connection(server_fd);
    free(session.clients);
}

int main(int argc, char *argv[]) {
    int port = 8080;
    if (argc > 1) {
        port = atoi(argv[1]);
    }
    run_server(port);
    return 0;
}