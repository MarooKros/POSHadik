#include "client.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Zobrazi stav hry v terminali
void display_game(const char *state) {
    system("clear");
    printf("Hra Hadik\n");
    char *copy = strdup(state);
    char *token = strtok(copy, ";");
    int width = 0, height = 0, elapsed = 0;
    
    if (token) {
        sscanf(token, "%d,%d", &width, &height);
        printf("Plocha: %dx%d\n", width, height);
    }
    
    char *find = strstr(state, "t");
    if (find) {
        sscanf(find + 1, "%d", &elapsed);
    }
    
    printf("Uplynulo: %ds\n", elapsed);
    printf("Stav: %s\n", state);
    printf("Ovladanie: WASD pohyb, P pauza, Q ukoncit\n");
    free(copy);
}

// Zobrazi hlavne menu s moznostami
void show_menu(int paused_available) {
    printf("\n=== Menu POSHadik ===\n");
    printf("1. Nova hra\n");
    printf("2. Pripojit sa\n");
    if (paused_available) printf("3. Pokracovat v pozastavenej hre\n");
    printf("9. Ukoncit\n");
    printf("Vyber: ");
}

// Dialog pre vytvorenie novej hry s parametrami
void create_new_game_menu(int sock) {
    int width = 20, height = 20, mode = 0, time_limit = 0, obstacles = 0;
    
    printf("Zadaj sirku plochy (1-50): ");
    scanf("%d", &width);
    if (width < 1) width = 1;
    if (width > 50) width = 50;
    
    printf("Zadaj vysku plochy (1-50): ");
    scanf("%d", &height);
    if (height < 1) height = 1;
    if (height > 50) height = 50;
    
    printf("Herny mod (0=standardny, 1=casovany): ");
    scanf("%d", &mode);
    if (mode != 1) mode = 0;
    
    if (mode == 1) {
        printf("Zadaj casovy limit (sekundy): ");
        scanf("%d", &time_limit);
        if (time_limit < 5) time_limit = 5;
    }
    
    printf("Prekazky (0=ziadne, 1=nahodne, 2=zo suboru obstacles.txt): ");
    scanf("%d", &obstacles);

    printf("Spustam novu hru %dx%d mod=%d cas=%d prekazky=%d\n", width, height, mode, time_limit, obstacles);
    
    char msg[256];
    sprintf(msg, "%s %d %d %d %d %d", MSG_NEW_GAME, width, height, mode, time_limit, obstacles);
    send_message(sock, msg);
    
    char *response = receive_message(sock);
    bool game_created = (response && strcmp(response, RSP_OK) == 0);
    if (response) free(response);
    
    send_message(sock, MSG_JOIN);
    char *join_response = receive_message(sock);
    
    if (game_created && join_response && strcmp(join_response, RSP_OK) == 0) {
        printf("Nova hra vytvorena. Si hrac 1.\n");
        printf("Pripojeny do hry. Pouzivaj WASD na pohyb, P na pauzu/pokracovanie, Q na ukoncenie.\n");
        printf("Ak chceš vidieť gui, hru treba zapnúť cez grafické rozhranie.\n");
        printf("wsl bash -c \"cd cesta/POSHadik/POSHadik && python3 gui_client.py {ip} {port}\"\n");
    } else if (!game_created) {
        printf("Nepodarilo sa vytvorit novu hru\n");
    } else {
        printf("Nepodarilo sa pripojit do hry\n");
    }
    
    if (join_response) free(join_response);
}

// Hlavna funkcia klienta - menu a hra
void run_client(const char *server_ip, int port) {
    int sock = init_ipc_client(server_ip, port);
    if (sock == -1) {
        fprintf(stderr, "Nepodarilo sa pripojit k serveru\n");
        return;
    }
    printf("Pripojeny k serveru %s:%d\n", server_ip, port);

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
                printf("Nepodarilo sa pripojit do hry\n");
                if (response) free(response);
                continue;
            }
            if (response) free(response);
        } else if (choice == 3 && paused) {
            send_message(sock, MSG_RESUME);
            paused = 0;
        } else if (choice == 9) {
            close_ipc_connection(sock);
            return;
        } else {
            printf("Neznamy vyber\n");
            continue;
        }

        printf("Pripojeny do hry. Pouzivaj WASD, P pre pauzu (navrat do menu), Q pre ukoncenie.\n");
        
        // Hlavna herna slucka
        while (1) {
            char key;
            scanf(" %c", &key);
            
            if (key == 'w' || key == 'W') send_message(sock, MSG_MOVE_UP);
            else if (key == 's' || key == 'S') send_message(sock, MSG_MOVE_DOWN);
            else if (key == 'a' || key == 'A') send_message(sock, MSG_MOVE_LEFT);
            else if (key == 'd' || key == 'D') send_message(sock, MSG_MOVE_RIGHT);
            else if (key == 'p' || key == 'P') {
                send_message(sock, MSG_PAUSE);
                paused = 1;
                break;
            }
            else if (key == 'q' || key == 'Q') {
                send_message(sock, MSG_LEAVE);
                close_ipc_connection(sock);
                return;
            }

            // Prijme odpoved zo servera
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
// client.c je cisto len pre testovanie pre spravne fungovanie hry treba spustit klienta v GUI 
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