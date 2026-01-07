#include "client.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <conio.h>
#else
#include <unistd.h>
#endif

void display_game(const char *state) {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
    printf("Snake Game\n");
#ifdef _WIN32
    char *copy = _strdup(state);
#else
    char *copy = strdup(state);
#endif
    char *token = strtok(copy, ";");
    if (token) {
        int width, height;
        sscanf(token, "%d,%d", &width, &height);
        printf("Board: %dx%d\n", width, height);
    }
    printf("State: %s\n", state);
    printf("Controls: WASD move, P pause, Q quit\n");
    free(copy);
}

void show_menu() {
    printf("Menu:\n");
    printf("1. New Game\n");
    printf("2. Join Game\n");
    printf("3. Exit\n");
    printf("Choose: ");
}

void create_new_game_menu(int sock) {
    int width, height, mode, time_limit = 0, obstacles = 0;
    printf("Enter game width (max 50): ");
    scanf("%d", &width);
    if (width > 50) width = 50;
    printf("Enter game height (max 50): ");
    scanf("%d", &height);
    if (height > 50) height = 50;
    printf("Game mode (0: standard, 1: timed): ");
    scanf("%d", &mode);
    if (mode == 1) {
        printf("Enter time limit (seconds): ");
        scanf("%d", &time_limit);
    }
    printf("Obstacles (0: no, 1: random, 2: from file): ");
    scanf("%d", &obstacles);
    char msg[256];
    sprintf(msg, "%s %d %d %d %d %d", MSG_NEW_GAME, width, height, mode, time_limit, obstacles);
    send_message(sock, msg);
    char *response = receive_message(sock);
    if (response && strcmp(response, RSP_OK) == 0) {
        printf("New game created. You are player 1.\n");
    } else {
        printf("Failed to create new game\n");
    }
    if (response) free(response);
    send_message(sock, MSG_JOIN);
    char *join_response = receive_message(sock);
    if (join_response && strcmp(join_response, RSP_OK) == 0) {
        printf("Joined game. Use WASD to move, P to pause/resume, Q to quit.\n");
    } else {
        printf("Failed to join game\n");
    }
    if (join_response) free(join_response);
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
    bool joined = false;
    if (choice == 1) {
        create_new_game_menu(sock);
        joined = true;
    } else if (choice == 2) {
        send_message(sock, MSG_JOIN);
        char *response = receive_message(sock);
        if (response && strcmp(response, RSP_OK) == 0) {
            joined = true;
            free(response);
        } else {
            printf("Failed to join game\n");
            if (response) free(response);
            close_ipc_connection(sock);
            return;
        }
    } else {
        close_ipc_connection(sock);
        return;
    }

    if (joined) {
        printf("Joined game. Use WASD to move, P to pause/resume, Q to quit.\n");
        bool paused = false;
        while (1) {
#ifdef _WIN32
            if (_kbhit()) {
                char key = _getch();
#else
            {
                char key;
                scanf(" %c", &key);
#endif
                switch (key) {
                    case 'w': case 'W': if (!paused) send_message(sock, MSG_MOVE_UP); break;
                    case 's': case 'S': if (!paused) send_message(sock, MSG_MOVE_DOWN); break;
                    case 'a': case 'A': if (!paused) send_message(sock, MSG_MOVE_LEFT); break;
                    case 'd': case 'D': if (!paused) send_message(sock, MSG_MOVE_RIGHT); break;
                    case 'p': case 'P': 
                        paused = !paused;
                        send_message(sock, paused ? MSG_PAUSE : MSG_RESUME);
                        break;
                    case 'q': case 'Q': send_message(sock, MSG_LEAVE); goto exit_game;
                }
            }
            char *msg = receive_message(sock);
            if (msg) {
                if (strcmp(msg, MSG_GAME_STATE) == 0) {
                    free(msg);
                    msg = receive_message(sock);
                    if (msg) {
                        display_game(msg);
                    }
                }
                if (msg) free(msg);
            }
#ifdef _WIN32
            Sleep(100);
#else
            usleep(100000);
#endif
        }
    }
exit_game:
    close_ipc_connection(sock);
}

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