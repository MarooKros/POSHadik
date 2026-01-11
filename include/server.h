#ifndef SERVER_H
#define SERVER_H

#include "game.h"
#include "ipc.h"
#include "messages.h"

// Struktura klienta
typedef struct {
    int client_socket;
    int snake_index;
    int game_id;
} Client;

// Hlavna session servera
typedef struct {
    Game games[10];
    int game_count;
    Client *clients;
    int num_clients;
    int max_clients;
} GameSession;

void run_server(int port);

#endif 