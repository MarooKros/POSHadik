#ifndef CLIENT_H
#define CLIENT_H

#include "game.h"
#include "ipc.h"

// Client functions
void run_client(const char *server_ip, int port);

#endif // CLIENT_H