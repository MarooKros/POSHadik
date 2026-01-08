#include "client.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Zobrazi aktualny stav hry na terminale - rozmery, cas, ploche a ovladanie
void display_game(const char *state) {
    system("clear");
    printf("Snake Game\n");
    char *copy = strdup(state);
    char *token = strtok(copy, ";");
    int width = 0, height = 0, elapsed = 0;
    if (token) {
        sscanf(token, "%d,%d", &width, &height);
        printf("Board: %dx%d\n", width, height);
    }
    char *find = strstr(state, "t");
    if (find) {
        sscanf(find + 1, "%d", &elapsed);
    }
    printf("Elapsed: %ds\n", elapsed);
    printf("State: %s\n", state);
    printf("Controls: WASD move, P pause, Q quit\n");
    free(copy);
}

// Vypise hlavne menu s moznostami: nova hra, pripojit sa, lokalna hra, pokracovat, koniec
void show_menu(int paused_available) {
    printf("\n=== POSHadik Menu ===\n");
    printf("1. New Game\n");
    printf("2. Join Game\n");
    printf("3. Local Game (2 players, 1 PC)\n");
    if (paused_available) printf("4. Continue paused game\n");
    printf("9. Exit\n");
    printf("Choose: ");
}

// Ziska parametre od hraca (sirka, vyska, mod, cas, prekazky) a vytvori novu hru na serveri
void create_new_game_menu(int sock) {
    int width = 20, height = 20, mode = 0, time_limit = 0, obstacles = 0;
    printf("Enter board width (1-50): ");
    scanf("%d", &width); if (width < 1) width = 1; if (width > 50) width = 50;
    printf("Enter board height (1-50): ");
    scanf("%d", &height); if (height < 1) height = 1; if (height > 50) height = 50;
    printf("Game mode (0=standard, 1=timed): ");
    scanf("%d", &mode); if (mode != 1) mode = 0;
    if (mode == 1) {
        printf("Enter time limit (seconds): ");
        scanf("%d", &time_limit);
        if (time_limit < 5) time_limit = 5;
    }
    printf("Obstacles (0=none, 1=random, 2=from file obstacles.txt): ");
    scanf("%d", &obstacles);

    printf("Starting new game %dx%d mode=%d time=%d obstacles=%d\n", width, height, mode, time_limit, obstacles);
    char msg[256];
    sprintf(msg, "%s %d %d %d %d %d", MSG_NEW_GAME, width, height, mode, time_limit, obstacles);
    send_message(sock, msg);
    char *response = receive_message(sock);
    bool game_created = (response && strcmp(response, RSP_OK) == 0);
    if (response) free(response);
    send_message(sock, MSG_JOIN);
    char *join_response = receive_message(sock);
    if (game_created && join_response && strcmp(join_response, RSP_OK) == 0) {
        printf("New game created. You are player 1.\n");
        printf("Joined game. Use WASD to move, P to pause/resume, Q to quit.\n");
    } else if (!game_created) {
        printf("Failed to create new game\n");
    } else {
        printf("Failed to join game\n");
    }
    if (join_response) free(join_response);
}

// Spusti lokalnu hru pre 2 hracov na jednom PC - pripoji 2 sockety, jeden ovlada WASD, druhy sipkami
void run_local_game(const char *server_ip, int port) {
    int sock1 = init_ipc_client(server_ip, port);
    int sock2 = init_ipc_client(server_ip, port);
    
    if (sock1 == -1 || sock2 == -1) {
        fprintf(stderr, "Failed to connect to server\n");
        return;
    }
    
    printf("Connected to server. Setting up local 2-player game...\n");
    
    int width = 20, height = 20;
    char msg[256];
    sprintf(msg, "%s %d %d %d %d %d", MSG_NEW_GAME, width, height, 0, 0, 0);
    send_message(sock1, msg);
    char *response = receive_message(sock1);
    if (response) free(response);
    
    send_message(sock1, MSG_JOIN);
    response = receive_message(sock1);
    if (response) free(response);
    
    send_message(sock2, MSG_JOIN);
    response = receive_message(sock2);
    if (response) free(response);
    
    printf("\nLocal Game Started!\n");
    printf("Player 1 (Green):  W/A/S/D to move\n");
    printf("Player 2 (Yellow): UP/DOWN/LEFT/RIGHT to move\n");
    printf("Press Q to quit\n\n");
    
    while (1) {
        {
            char key;
            if (scanf("%c", &key) <= 0) key = 0;
            
            if (key == 'q' || key == 'Q') {
                send_message(sock1, MSG_LEAVE);
                send_message(sock2, MSG_LEAVE);
                break;
            }
            
            switch (key) {
                case 'w': case 'W': send_message(sock1, MSG_MOVE_UP); break;
                case 's': case 'S': send_message(sock1, MSG_MOVE_DOWN); break;
                case 'a': case 'A': send_message(sock1, MSG_MOVE_LEFT); break;
                case 'd': case 'D': send_message(sock1, MSG_MOVE_RIGHT); break;
            }
        }
        
        char *msg1 = receive_message(sock1);
        if (msg1) {
            if (strncmp(msg1, MSG_GAME_STATE, strlen(MSG_GAME_STATE)) == 0) {
                display_game(msg1 + strlen(MSG_GAME_STATE));
            }
            free(msg1);
        }
        
        char *msg2 = receive_message(sock2);
        if (msg2) {
            free(msg2);
        }
        
        usleep(50000);
    }
    
    close_ipc_connection(sock1);
    close_ipc_connection(sock2);
}

// Hlavna klientska logika: pripoji sa na server, zobrazi menu, ovlada hru, prijima stavy a odosiela prikazy
void run_client(const char *server_ip, int port) {
    int sock = init_ipc_client(server_ip, port);
    if (sock == -1) {
        fprintf(stderr, "Failed to connect to server\n");
        return;
    }
    printf("Connected to server %s:%d\n", server_ip, port);

    int paused = 0;

    while (1) {
        show_menu(paused);
        int choice = 0;
        scanf("%d", &choice);

        if (choice == 1) {
            create_new_game_menu(sock);
        } else if (choice == 2) {
            send_message(sock, MSG_JOIN);
            char *response = receive_message(sock);
            if (!(response && strcmp(response, RSP_OK) == 0)) {
                printf("Failed to join game\n");
                if (response) free(response);
                continue;
            }
            if (response) free(response);
        } else if (choice == 3) {
            // local game uses separate sockets
            run_local_game(server_ip, port);
            continue;
        } else if (choice == 4 && paused) {
            // continue paused game
            send_message(sock, MSG_RESUME);
            paused = 0;
        } else if (choice == 9) {
            close_ipc_connection(sock);
            return;
        } else {
            printf("Unknown choice\n");
            continue;
        }

        // gameplay loop
        printf("Joined game. Use WASD, P pause (back to menu), Q quit.\n");
        while (1) {
            char key;
            scanf(" %c", &key);
            if (key == 'w' || key == 'W') send_message(sock, MSG_MOVE_UP);
            else if (key == 's' || key == 'S') send_message(sock, MSG_MOVE_DOWN);
            else if (key == 'a' || key == 'A') send_message(sock, MSG_MOVE_LEFT);
            else if (key == 'd' || key == 'D') send_message(sock, MSG_MOVE_RIGHT);
            else if (key == 'p' || key == 'P') { send_message(sock, MSG_PAUSE); paused = 1; break; }
            else if (key == 'q' || key == 'Q') { send_message(sock, MSG_LEAVE); close_ipc_connection(sock); return; }

            char *msg = receive_message(sock);
            if (msg) {
                if (strcmp(msg, MSG_GAME_STATE) == 0) {
                    free(msg);
                    msg = receive_message(sock);
                    if (msg) display_game(msg);
                }
                if (msg) free(msg);
            }
            usleep(80000);
        }
    }
}

// Hlavny vstupny bod klienta - nacita IP a port, spusti klienta
int main(int argc, char *argv[]) {
    char *server_ip = "127.0.0.1";
    int port = 8080;
    if (argc > 1) {
        server_ip = argv[1];
    }
    if (argc > 2) {
        port = atoi(argv[2]);
    }
    run_client(server_ip, port);
    return 0;
}