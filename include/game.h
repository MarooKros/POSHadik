#ifndef GAME_H
#define GAME_H

#include <stdbool.h>

typedef struct {
    int x, y;
} Position;

typedef struct {
    Position *body;
    int length;
    int direction; // 0 up, 1 right, 2 down, 3 left
    int sleep_until; // timestamp when snake can move again
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
    int game_mode; // 0 standard, 1 timed
    int time_limit; // seconds for timed mode
    int start_time; // game start timestamp
    int empty_since; // when the last snake disappeared (standard mode)
    int freeze_until; // global freeze (e.g., 3s after join)
    int scores[10];
    bool game_over;
    int paused; 
} Game;

void init_game(Game *game, int width, int height, bool has_obstacles, int game_mode, int time_limit);
void update_game(Game *game);
bool is_collision(Game *game, Snake *snake);
void generate_fruit(Game *game);
void generate_obstacles(Game *game);
void move_snake(Snake *snake);
void change_direction(Snake *snake, int new_direction);
void add_snake(Game *game, int start_x, int start_y);
void remove_snake(Game *game, int index);
char* serialize_game_state(Game *game);
void load_obstacles_from_file(Game *game, const char *filename);

#endif 