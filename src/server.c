#include "server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

GameSession session;

void process_message(int client_index, const char *message) {
    Client *client = &session.clients[client_index];
    char cmd[256];
    strncpy(cmd, message, sizeof(cmd) - 1);
    cmd[sizeof(cmd) - 1] = '\0';
    char *token = strtok(cmd, " ");
    
    if (strcmp(token, "LIST_GAMES") == 0) {
        char response[4096] = "GAMES";
        for (int i = 0; i < session.game_count; i++) {
            Game *g = &session.games[i];
            if (g->width > 0 && !g->game_over) {
                int alive = 0;
                for (int j = 0; j < g->num_snakes; j++) {
                    if (g->snakes[j].length > 0) alive++;
                }
                char buf[128];
                sprintf(buf, " %d:%d:%d:%d;", i, g->width, g->height, alive);
                strcat(response, buf);
            }
        }
        send_message(client->client_socket, response);
        return;
    }
    
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
        
        int game_id = session.game_count;
        if (game_id >= 10) {
            send_message(client->client_socket, "ERROR TOO_MANY_GAMES");
            return;
        }
        
        Game *game = &session.games[game_id];
        free(game->snakes);
        free(game->fruits);
        free(game->obstacles);
        init_game(game, width, height, obstacles > 0, mode, time_limit);
        game->creator_socket = client->client_socket;
        if (obstacles == 1) {
            generate_obstacles(game);
        } else if (obstacles == 2) {
            load_obstacles_from_file(game, "obstacles.txt");
        }
        add_snake(game, rand() % game->width, rand() % game->height);
        client->snake_index = 0;
        client->game_id = game_id;
        session.game_count++;
        
        char response[64];
        sprintf(response, "OK %d %d", client->snake_index + 1, game_id);
        send_message(client->client_socket, response);
        return;
    } else if (strcmp(token, MSG_JOIN) == 0) {
        token = strtok(NULL, " ");
        if (!token) {
            send_message(client->client_socket, "ERROR NO_GAME_ID");
            return;
        }
        int game_id = atoi(token);
        
        if (game_id < 0 || game_id >= session.game_count || session.games[game_id].width == 0 || session.games[game_id].game_over) {
            send_message(client->client_socket, "ERROR NO_GAME");
            return;
        }
        
        if (client->snake_index != -1) {
            send_message(client->client_socket, RSP_FULL);
            return;
        }
        
        Game *game = &session.games[game_id];
        int empty_slot = -1;
        for (int i = 0; i < game->num_snakes; i++) {
            if (game->snakes[i].length == 0) {
                empty_slot = i;
                break;
            }
        }
        
        if (empty_slot != -1) {
            revive_snake(game, empty_slot, rand() % game->width, rand() % game->height);
            client->snake_index = empty_slot;
            client->game_id = game_id;
            game->freeze_until = (int)time(NULL) + 3;
            char response[32];
            sprintf(response, "OK %d", client->snake_index + 1);
            send_message(client->client_socket, response);
        } else if (game->num_snakes < session.max_clients) {
            add_snake(game, rand() % game->width, rand() % game->height);
            client->snake_index = game->num_snakes - 1;
            client->game_id = game_id;
            game->freeze_until = (int)time(NULL) + 3;
            char response[32];
            sprintf(response, "OK %d", client->snake_index + 1);
            send_message(client->client_socket, response);
        } else {
            send_message(client->client_socket, RSP_FULL);
        }
    } else if (strcmp(message, MSG_LEAVE) == 0) {
        if (client->snake_index != -1 && client->game_id >= 0 && client->game_id < session.game_count) {
            remove_snake(&session.games[client->game_id], client->snake_index);
            client->snake_index = -1;
            client->game_id = -1;
        }
        send_message(client->client_socket, RSP_OK);
    } else if (strcmp(message, MSG_PAUSE) == 0) {
        if (client->game_id >= 0 && client->game_id < session.game_count) {
            session.games[client->game_id].paused = 1;
        }
        send_message(client->client_socket, RSP_OK);
    } else if (strcmp(message, MSG_RESUME) == 0) {
        if (client->game_id >= 0 && client->game_id < session.game_count) {
            session.games[client->game_id].paused = 0;
        }
        send_message(client->client_socket, RSP_OK);
    } else if (client->snake_index != -1 && client->game_id >= 0 && client->game_id < session.game_count) {
        Game *game = &session.games[client->game_id];
        Snake *snake = &game->snakes[client->snake_index];
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

void* handle_client(void* lpParam) {
    int client_socket = (int)(intptr_t)lpParam;
    int client_index = -1;
    for (int i = 0; i < session.num_clients; i++) {
        if (session.clients[i].client_socket == client_socket) {
            client_index = i;
            break;
        }
    }
    if (client_index == -1) 
        return NULL;

    while (1) {
        char *msg = receive_message(client_socket);
        if (msg == NULL) break;
        process_message(client_index, msg);
        free(msg);
    }
    
    for (int i = 0; i < session.game_count; i++) {
        if (session.games[i].creator_socket == client_socket) {
            session.games[i].game_over = true;
            printf("Hra %d bola ukoncena, pretoze tvorca hry sa odpojil.\n", i + 1);
        }
    }
    
    if (session.clients[client_index].snake_index != -1 && session.clients[client_index].game_id >= 0) {
        remove_snake(&session.games[session.clients[client_index].game_id], session.clients[client_index].snake_index);
    }
    close_ipc_connection(client_socket);
    return NULL;
}

void* game_update_thread(void* lpParam) {
    while (1) {
        usleep(150000);
        for (int game_id = 0; game_id < session.game_count; game_id++) {
            Game *game = &session.games[game_id];
            if (game->width == 0) continue;
            
            update_game(game);
            char *state = serialize_game_state(game);
            
            for (int i = 0; i < session.num_clients; i++) {
                if (session.clients[i].game_id == game_id && 
                    (session.clients[i].snake_index != -1 || game->game_over)) {
                    send_message(session.clients[i].client_socket, MSG_GAME_STATE);
                    send_message(session.clients[i].client_socket, state);
                }
            }
            free(state);
        }
    }
    return NULL;
}

void run_server(int port) {
    session.num_clients = 0;
    session.max_clients = 10;
    session.game_count = 0;
    session.clients = malloc(session.max_clients * sizeof(Client));
    for (int i = 0; i < 10; i++) {
        init_game(&session.games[i], 0, 0, false, 0, 0);
    }
    
    pthread_t update_thread;
    pthread_create(&update_thread, NULL, game_update_thread, NULL);

    int server_fd = init_ipc_server(port);
    if (server_fd == -1) {
        fprintf(stderr, "Nepodarilo sa inicializovat server\n");
        return;
    }
    printf("Server spusteny na porte %d\n", port);
    while (1) {
        struct sockaddr_in client_addr;
        int addr_len = sizeof(client_addr);
        printf("Cakam na pripojenie...\n");
        int client_socket = accept(server_fd, (struct sockaddr*)&client_addr, &addr_len);
        if (client_socket < 0) {
            perror("accept");
            continue;
        }
        printf("Prijate pripojenie\n");
        if (session.num_clients < session.max_clients) {
            session.clients[session.num_clients].client_socket = client_socket;
            session.clients[session.num_clients].snake_index = -1;
            session.clients[session.num_clients].game_id = -1;
            session.num_clients++;
            pthread_t thread;
            pthread_create(&thread, NULL, handle_client, (void*)(intptr_t)client_socket);
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