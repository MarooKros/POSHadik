#include "game.h"
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>

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
    game->start_time = time(NULL);
    game->game_over = false;
    memset(game->scores, 0, sizeof(game->scores));
    srand(time(NULL));
    if (has_obstacles) {
        generate_obstacles(game);
    }
}

void generate_obstacles(Game *game) {
    if (!game->has_obstacles) return;
    int num_obstacles = (game->width * game->height) / 10;
    if (num_obstacles > 100) num_obstacles = 100;
    game->obstacles = malloc(num_obstacles * sizeof(Obstacle));
    for (int i = 0; i < num_obstacles; i++) {
        Position pos;
        bool valid = false;
        int attempts = 0;
        while (!valid && attempts < 100) {
            pos.x = rand() % game->width;
            pos.y = rand() % game->height;
            valid = true;
            attempts++;
        }
        if (valid) {
            game->obstacles[i].pos = pos;
            game->num_obstacles++;
        }
    }
}

void update_game(Game *game) {
    if (game->game_over) return;
    if (game->game_mode == 1 && time(NULL) - game->start_time >= game->time_limit) {
        game->game_over = true;
        return;
    }
    for (int i = 0; i < game->num_snakes; i++) {
        move_snake(&game->snakes[i]);
        if (is_collision(game, &game->snakes[i])) {
            game->scores[i] = game->snakes[i].length - 1;
            remove_snake(game, i);
            i--;
        } else {
            Position head = game->snakes[i].body[0];
            for (int j = 0; j < game->num_fruits; j++) {
                if (head.x == game->fruits[j].pos.x && head.y == game->fruits[j].pos.y) {
                    int old_len = game->snakes[i].length;
                    Position tail = game->snakes[i].body[old_len - 1];
                    game->snakes[i].length++;
                    game->snakes[i].body = realloc(game->snakes[i].body, game->snakes[i].length * sizeof(Position));
                    game->snakes[i].body[game->snakes[i].length - 1] = tail;
                    for (int k = j; k < game->num_fruits - 1; k++) {
                        game->fruits[k] = game->fruits[k + 1];
                    }
                    game->num_fruits--;
                    game->fruits = realloc(game->fruits, game->num_fruits * sizeof(Fruit));
                    generate_fruit(game);
                    break;
                }
            }
        }
    }
    if (game->num_snakes == 0 || (game->game_mode == 0 && time(NULL) - game->start_time > 10 && game->num_snakes < 1)) {
        game->game_over = true;
    }
}

bool is_collision(Game *game, Snake *snake) {
    Position head = snake->body[0];
    if (head.x < 0 || head.x >= game->width || head.y < 0 || head.y >= game->height) {
        return true;
    }
    for (int i = 1; i < snake->length; i++) {
        if (head.x == snake->body[i].x && head.y == snake->body[i].y) {
            return true;
        }
    }
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
    if (game->num_fruits >= game->num_snakes) return;
    Position pos;
    bool valid = false;
    int attempts = 0;
    while (!valid && attempts < 100) {
        pos.x = rand() % game->width;
        pos.y = rand() % game->height;
        valid = true;
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
    for (int i = snake->length - 1; i > 0; i--) {
        snake->body[i] = snake->body[i-1];
    }
    switch (snake->direction) {
        case 0: snake->body[0].y--; break;
        case 1: snake->body[0].x++; break;
        case 2: snake->body[0].y++; break;
        case 3: snake->body[0].x--; break;
    }
}

void change_direction(Snake *snake, int new_direction) {
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
    generate_fruit(game);
}

void remove_snake(Game *game, int index) {
    if (index < 0 || index >= game->num_snakes) return;
    free(game->snakes[index].body);
    for (int i = index; i < game->num_snakes - 1; i++) {
        game->snakes[i] = game->snakes[i + 1];
    }
    game->num_snakes--;
    game->snakes = realloc(game->snakes, game->num_snakes * sizeof(Snake));
    if (game->num_fruits > game->num_snakes) {
        game->num_fruits--;
        game->fruits = realloc(game->fruits, game->num_fruits * sizeof(Fruit));
    }
}

char* serialize_game_state(Game *game) {
    char buffer[131072];
    sprintf(buffer, "%d,%d;", game->width, game->height);
    for (int i = 0; i < game->num_snakes; i++) {
        Snake *s = &game->snakes[i];
        sprintf(buffer + strlen(buffer), "s%d,%d", s->length, s->direction);
        for (int j = 0; j < s->length; j++) {
            sprintf(buffer + strlen(buffer), ",%d,%d", s->body[j].x, s->body[j].y);
        }
        sprintf(buffer + strlen(buffer), ";");
    }
    for (int i = 0; i < game->num_fruits; i++) {
        sprintf(buffer + strlen(buffer), "f%d,%d;", game->fruits[i].pos.x, game->fruits[i].pos.y);
    }
    for (int i = 0; i < game->num_obstacles; i++) {
        sprintf(buffer + strlen(buffer), "o%d,%d;", game->obstacles[i].pos.x, game->obstacles[i].pos.y);
    }
    sprintf(buffer + strlen(buffer), "sc");
    for (int i = 0; i < 10; i++) {
        sprintf(buffer + strlen(buffer), ",%d", game->scores[i]);
    }
    sprintf(buffer + strlen(buffer), ";");
    sprintf(buffer + strlen(buffer), "go%d;", game->game_over ? 1 : 0);
#ifdef _WIN32
    return _strdup(buffer);
#else
    return strdup(buffer);
#endif
}

void load_obstacles_from_file(Game *game, const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("fopen");
        return;
    }
    int x, y;
    while (fscanf(file, "%d %d", &x, &y) == 2) {
        if (game->num_obstacles < 100) {
            game->obstacles = realloc(game->obstacles, (game->num_obstacles + 1) * sizeof(Obstacle));
            game->obstacles[game->num_obstacles].pos.x = x;
            game->obstacles[game->num_obstacles].pos.y = y;
            game->num_obstacles++;
        }
    }
    fclose(file);
}