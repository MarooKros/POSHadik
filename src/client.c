#include "client.h"
#include <stdio.h>
#include <stdlib.h>

#include "client.h"
#include <stdio.h>
#include <stdlib.h>

void show_menu() {
    printf("Menu:\n");
    printf("1. New Game\n");
    printf("2. Join Game\n");
    printf("3. Exit\n");
    printf("Choose: ");
}

void run_client(const char *server_ip, int port) {
    int sock = init_ipc_client(server_ip, port);
    if (sock == -1) {
        fprintf(stderr, "Failed to connect to server\n");
        return;
    }
    printf("Connected to server %s:%d\n", server_ip, port);
    // Simple menu loop
    while (1) {
        show_menu();
        int choice;
        scanf("%d", &choice);
        switch (choice) {
            case 1:
                send_message(sock, "NEW_GAME");
                break;
            case 2:
                send_message(sock, "JOIN_GAME");
                break;
            case 3:
                send_message(sock, "EXIT");
                close_ipc_connection(sock);
                return;
            default:
                printf("Invalid choice\n");
        }
        // Receive response
        char *response = receive_message(sock);
        if (response) {
            printf("Server: %s\n", response);
            free(response);
        }
    }
}