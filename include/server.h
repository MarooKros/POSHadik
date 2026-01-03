#ifndef SERVER_H
#define SERVER_H

#include "game.h"
#include "ipc.h"

typedef struct {
    int client_socket;
    int snake_index; // -1 if not in game
} Client;

typedef struct {
    Game game;
    Client *clients;
    int num_clients;
    int max_clients;
} GameSession;

// Server functions
void run_server(int port);

#endif // SERVER_H