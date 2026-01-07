#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <process.h>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")

#define ID_TIMER 1
#define SCALE 10

HWND hwnd;
SOCKET sock;
char game_state[131072];
int width, height;

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_KEYDOWN:
            if (sock != INVALID_SOCKET) {
                char msg[20];
                switch (wParam) {
                    case 'W': strcpy(msg, "MOVE_UP\n"); break;
                    case 'S': strcpy(msg, "MOVE_DOWN\n"); break;
                    case 'A': strcpy(msg, "MOVE_LEFT\n"); break;
                    case 'D': strcpy(msg, "MOVE_RIGHT\n"); break;
                    case 'P': strcpy(msg, "PAUSE\n"); break;
                    case 'Q': strcpy(msg, "LEAVE\n"); break;
                    default: return 0;
                }
                send(sock, msg, strlen(msg), 0);
            }
            break;
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            HBRUSH hBrushGreen = CreateSolidBrush(RGB(0, 255, 0));
            HBRUSH hBrushRed = CreateSolidBrush(RGB(255, 0, 0));
            HBRUSH hBrushGray = CreateSolidBrush(RGB(128, 128, 128));
            char *parts = strtok(game_state, ";");
            if (parts) {
                sscanf(parts, "%d,%d", &width, &height);
                parts = strtok(NULL, ";");
                while (parts) {
                    if (parts[0] == 's') {
                        char *data = parts + 1;
                        int len, dir;
                        sscanf(data, "%d,%d", &len, &dir);
                        data = strchr(data, ',') + 1;
                        data = strchr(data, ',') + 1;
                        for (int i = 0; i < len; i++) {
                            int x, y;
                            sscanf(data, "%d,%d", &x, &y);
                            RECT rect = {x * SCALE, y * SCALE, (x + 1) * SCALE, (y + 1) * SCALE};
                            FillRect(hdc, &rect, hBrushGreen);
                            data = strchr(data, ',') + 1;
                            data = strchr(data, ',') + 1;
                        }
                    } else if (parts[0] == 'f') {
                        int x, y;
                        sscanf(parts + 1, "%d,%d", &x, &y);
                        RECT rect = {x * SCALE, y * SCALE, (x + 1) * SCALE, (y + 1) * SCALE};
                        FillRect(hdc, &rect, hBrushRed);
                    } else if (parts[0] == 'o') {
                        int x, y;
                        sscanf(parts + 1, "%d,%d", &x, &y);
                        RECT rect = {x * SCALE, y * SCALE, (x + 1) * SCALE, (y + 1) * SCALE};
                        FillRect(hdc, &rect, hBrushGray);
                    }
                    parts = strtok(NULL, ";");
                }
            }
            DeleteObject(hBrushGreen);
            DeleteObject(hBrushRed);
            DeleteObject(hBrushGray);
            EndPaint(hwnd, &ps);
            break;
        }
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

void receive_thread(void *arg) {
    while (1) {
        char buffer[131072];
        int len = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (len > 0) {
            buffer[len] = '\0';
            if (strstr(buffer, "GAME_STATE")) {
                char *state = strstr(buffer, "GAME_STATE") + 10;
                strcpy(game_state, state);
                InvalidateRect(hwnd, NULL, TRUE);
            }
        } else {
            break;
        }
    }
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(8080);
    inet_pton(AF_INET, "172.22.52.72", &serv_addr.sin_addr);
    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) != 0) {
        MessageBox(NULL, "Failed to connect", "Error", MB_OK);
        return 1;
    }
    send(sock, "JOIN\n", 5, 0);

    WNDCLASS wc = {0};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "SnakeGUI";
    RegisterClass(&wc);

    hwnd = CreateWindow("SnakeGUI", "Snake Game", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 600, 600, NULL, NULL, hInstance, NULL);
    ShowWindow(hwnd, nCmdShow);

    _beginthread(receive_thread, 0, NULL);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    closesocket(sock);
    WSACleanup();
    return 0;
}