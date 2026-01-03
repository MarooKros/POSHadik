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
}

void generate_obstacles(Game *game) {
    if (!game->has_obstacles) return;
    // Simple random obstacles, ensure path exists
    int num_obstacles = (game->width * game->height) / 10; // 10% obstacles
    game->obstacles = malloc(num_obstacles * sizeof(Obstacle));
    for (int i = 0; i < num_obstacles; i++) {
        Position pos;
        bool valid = false;
        int attempts = 0;
        while (!valid && attempts < 100) {
            pos.x = rand() % game->width;
            pos.y = rand() % game->height;
            valid = true;
            // Avoid edges or something, but for simplicity, just random
            attempts++;
        }
        if (valid) {
            game->obstacles[i].pos = pos;
            game->num_obstacles++;
        }
    }
}

void update_game(Game *game) {
    // Update each snake
    for (int i = 0; i < game->num_snakes; i++) {
        move_snake(&game->snakes[i]);
        if (is_collision(game, &game->snakes[i])) {
            // Handle collision - remove snake or mark as dead
            // For now, just print
            printf("Snake %d collided\n", i);
        } else {
            // Check if ate fruit
            Position head = game->snakes[i].body[0];
            for (int j = 0; j < game->num_fruits; j++) {
                if (head.x == game->fruits[j].pos.x && head.y == game->fruits[j].pos.y) {
                    // Grow snake
                    game->snakes[i].length++;
                    game->snakes[i].body = realloc(game->snakes[i].body, game->snakes[i].length * sizeof(Position));
                    // Remove fruit
                    for (int k = j; k < game->num_fruits - 1; k++) {
                        game->fruits[k] = game->fruits[k + 1];
                    }
                    game->num_fruits--;
                    game->fruits = realloc(game->fruits, game->num_fruits * sizeof(Fruit));
                    // Generate new fruit
                    generate_fruit(game);
                    break;
                }
            }
        }
    }
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
    for (int i = 0; i < game->num_snakes; i++) {
        if (&game->snakes[i] != snake) {
            for (int j = 0; j < game->snakes[i].length; j++) {
                if (head.x == game->snakes[i].body[j].x && head.y == game->snakes[i].body[j].y) {
                    return true;
                }
            }
        }
    }
    for (int i = 0; i < game->num_obstacles; i++) {
        if (head.x == game->obstacles[i].pos.x && head.y == game->obstacles[i].pos.y) {
            return true;
        }
    }
    return false;
}

void generate_fruit(Game *game) {
    if (game->num_fruits >= game->num_snakes) return; // Max fruits = num snakes
    Position pos;
    bool valid = false;
    int attempts = 0;
    while (!valid && attempts < 100) {
        pos.x = rand() % game->width;
        pos.y = rand() % game->height;
        valid = true;
        // Check if position is free
        for (int i = 0; i < game->num_snakes; i++) {
            for (int j = 0; j < game->snakes[i].length; j++) {
                if (game->snakes[i].body[j].x == pos.x && game->snakes[i].body[j].y == pos.y) {
                    valid = false;
                    break;
                }
            }
            if (!valid) break;
        }
        if (valid) {
            for (int i = 0; i < game->num_obstacles; i++) {
                if (game->obstacles[i].pos.x == pos.x && game->obstacles[i].pos.y == pos.y) {
                    valid = false;
                    break;
                }
            }
        }
        if (valid) {
            for (int i = 0; i < game->num_fruits; i++) {
                if (game->fruits[i].pos.x == pos.x && game->fruits[i].pos.y == pos.y) {
                    valid = false;
                    break;
                }
            }
        }
        attempts++;
    }
    if (valid) {
        game->fruits = realloc(game->fruits, (game->num_fruits + 1) * sizeof(Fruit));
        game->fruits[game->num_fruits].pos = pos;
        game->num_fruits++;
    }
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

void add_snake(Game *game, int start_x, int start_y) {
    game->snakes = realloc(game->snakes, (game->num_snakes + 1) * sizeof(Snake));
    Snake *snake = &game->snakes[game->num_snakes];
    snake->length = 1;
    snake->body = malloc(sizeof(Position));
    snake->body[0].x = start_x;
    snake->body[0].y = start_y;
    snake->direction = 1; // right
    game->num_snakes++;
    generate_fruit(game); // Ensure fruits = snakes
}

void remove_snake(Game *game, int index) {
    if (index < 0 || index >= game->num_snakes) return;
    free(game->snakes[index].body);
    for (int i = index; i < game->num_snakes - 1; i++) {
        game->snakes[i] = game->snakes[i + 1];
    }
    game->num_snakes--;
    game->snakes = realloc(game->snakes, game->num_snakes * sizeof(Snake));
    // Remove extra fruit if any
    if (game->num_fruits > game->num_snakes) {
        game->num_fruits--;
        game->fruits = realloc(game->fruits, game->num_fruits * sizeof(Fruit));
    }
}