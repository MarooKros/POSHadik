#ifndef SERVER_H
#define SERVER_H

#include "game.h"
#include "ipc.h"
#include "messages.h"

typedef struct {
    int client_socket;
    int snake_index;
} Client;

typedef struct {
    Game game;
    Client *clients;
    int num_clients;
    int max_clients;
} GameSession;

void run_server(int port);

#endif 