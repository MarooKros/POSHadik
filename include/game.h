#ifndef GAME_H
#define GAME_H

#include <stdbool.h>

// Define game structures
typedef struct {
    int x, y;
} Position;

typedef struct {
    Position *body;
    int length;
    int direction; // 0: up, 1: right, 2: down, 3: left
} Snake;

typedef struct {
    Position pos;
} Fruit;

typedef struct {
    Position pos;
} Obstacle;

typedef struct {
    int width, height;
    Snake *snakes;
    int num_snakes;
    Fruit *fruits;
    int num_fruits;
    Obstacle *obstacles;
    int num_obstacles;
    bool has_obstacles;
    int game_mode; // 0: standard, 1: timed
    int time_limit; // in seconds
} Game;

// Function prototypes
void init_game(Game *game, int width, int height, bool has_obstacles, int game_mode, int time_limit);
void update_game(Game *game);
bool is_collision(Game *game, Snake *snake);
void generate_fruit(Game *game);
void move_snake(Snake *snake);
void change_direction(Snake *snake, int new_direction);

#endif // GAME_H