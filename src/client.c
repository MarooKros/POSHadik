#include "client.h"
#include <stdio.h>
#include <stdlib.h>

#include "client.h"
#include <stdio.h>
#include <stdlib.h>
#include <conio.h> 

void show_menu() {
    printf("Menu:\n");
    printf("1. Join Game\n");
    printf("2. Exit\n");
    printf("Choose: ");
}

void run_client(const char *server_ip, int port) {
    int sock = init_ipc_client(server_ip, port);
    if (sock == -1) {
        fprintf(stderr, "Failed to connect to server\n");
        return;
    }
    printf("Connected to server %s:%d\n", server_ip, port);

    show_menu();
    int choice;
    scanf("%d", &choice);
    if (choice == 1) {
        send_message(sock, MSG_JOIN);
        char *response = receive_message(sock);
        if (response && strcmp(response, RSP_OK) == 0) {
            printf("Joined game. Use WASD to move, Q to quit.\n");
            free(response);
            // Game loop
            while (1) {
                if (_kbhit()) {
                    char key = _getch();
                    switch (key) {
                        case 'w': case 'W': send_message(sock, MSG_MOVE_UP); break;
                        case 's': case 'S': send_message(sock, MSG_MOVE_DOWN); break;
                        case 'a': case 'A': send_message(sock, MSG_MOVE_LEFT); break;
                        case 'd': case 'D': send_message(sock, MSG_MOVE_RIGHT); break;
                        case 'q': case 'Q': send_message(sock, MSG_LEAVE); goto exit_game;
                    }
                }
                // Receive game state
                char *msg = receive_message(sock);
                if (msg) {
                    if (strcmp(msg, MSG_GAME_STATE) == 0) {
                        // TODO: Parse and display game state
                        printf("Game update received\n");
                    }
                    free(msg);
                }
                Sleep(100); // Small delay
            }
        } else {
            printf("Failed to join game\n");
            if (response) free(response);
        }
    } else if (choice == 2) {
        send_message(sock, MSG_LEAVE);
    }
exit_game:
    close_ipc_connection(sock);
}