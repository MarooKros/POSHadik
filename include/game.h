#ifndef GAME_H
#define GAME_H

#include <stdbool.h>

// Pozicia v hre (x, y suradnice)
typedef struct {
    int x, y;
} Position;

// Had - telo, smer, stav 
typedef struct {
    Position *body;
    int length;
    int direction;
    int sleep_until; 
    bool paused; 
} Snake;

// Ovocie na mape
typedef struct {
    Position pos;
} Fruit;

// Prekazka na mape
typedef struct {
    Position pos;
} Obstacle;

// Hlavna struktura hry
typedef struct {
    int width, height;
    Snake *snakes;
    int num_snakes;
    Fruit *fruits;
    int num_fruits;
    Obstacle *obstacles;
    int num_obstacles;
    bool has_obstacles;
    int game_mode; 
    int time_limit; 
    int start_time;
    int empty_since; 
    int freeze_until;
    int scores[10];
    bool game_over;
    int paused;
    int creator_socket;
} Game;

void init_game(Game *game, int width, int height, bool has_obstacles, int game_mode, int time_limit);
void update_game(Game *game);
bool is_collision(Game *game, Snake *snake);
void generate_fruit(Game *game);
void generate_obstacles(Game *game);
void move_snake(Snake *snake);
void change_direction(Snake *snake, int new_direction);
void add_snake(Game *game, int start_x, int start_y);
void revive_snake(Game *game, int index, int start_x, int start_y);
void remove_snake(Game *game, int index);
char* serialize_game_state(Game *game);
void load_obstacles_from_file(Game *game, const char *filename);

#endif 