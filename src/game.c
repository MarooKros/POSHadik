#include "game.h"
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

void init_game(Game *game, int width, int height, bool has_obstacles, int game_mode, int time_limit) {
    game->width = width;
    game->height = height;
    game->has_obstacles = has_obstacles;
    game->game_mode = game_mode;
    game->time_limit = time_limit;
    game->num_snakes = 0;
    game->snakes = NULL;
    game->num_fruits = 0;
    game->fruits = NULL;
    game->num_obstacles = 0;
    game->obstacles = NULL;
    srand(time(NULL));
    // Initialize obstacles if needed
    if (has_obstacles) {
        // TODO: Generate obstacles
    }
}

void update_game(Game *game) {
    // Update each snake
    for (int i = 0; i < game->num_snakes; i++) {
        move_snake(&game->snakes[i]);
        if (is_collision(game, &game->snakes[i])) {
            // Handle collision
        }
    }
    // Check if fruits are eaten, generate new ones
}

bool is_collision(Game *game, Snake *snake) {
    Position head = snake->body[0];
    // Check walls
    if (head.x < 0 || head.x >= game->width || head.y < 0 || head.y >= game->height) {
        return true;
    }
    // Check self collision
    for (int i = 1; i < snake->length; i++) {
        if (head.x == snake->body[i].x && head.y == snake->body[i].y) {
            return true;
        }
    }
    // Check other snakes and obstacles
    // TODO: Implement
    return false;
}

void generate_fruit(Game *game) {
    // TODO: Implement fruit generation
}

void move_snake(Snake *snake) {
    // Move body
    for (int i = snake->length - 1; i > 0; i--) {
        snake->body[i] = snake->body[i-1];
    }
    // Move head
    switch (snake->direction) {
        case 0: snake->body[0].y--; break; // up
        case 1: snake->body[0].x++; break; // right
        case 2: snake->body[0].y++; break; // down
        case 3: snake->body[0].x--; break; // left
    }
}

void change_direction(Snake *snake, int new_direction) {
    // Prevent reverse direction
    if ((snake->direction + 2) % 4 != new_direction) {
        snake->direction = new_direction;
    }
}