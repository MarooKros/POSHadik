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
    game->start_time = (int)time(NULL);
    game->empty_since = 0;
    game->freeze_until = 0;

    game->num_snakes = 0;
    game->snakes = NULL;
    game->num_fruits = 0;
    game->fruits = NULL;
    game->num_obstacles = 0;
    game->obstacles = NULL;
    game->game_over = false;
    game->paused = 0;
    game->creator_socket = -1;
    memset(game->scores, 0, sizeof(game->scores));
    srand((unsigned)time(NULL));

    if (has_obstacles) {
        generate_obstacles(game);
    }
}

void generate_obstacles(Game *game) {
    if (!game->has_obstacles) return;
    int max_obs = (game->width * game->height) / 10;
    if (max_obs < 1) max_obs = 1;
    if (max_obs > 100) max_obs = 100;
    game->obstacles = malloc(max_obs * sizeof(Obstacle));
    game->num_obstacles = 0;

    int attempts = 0;
    while (game->num_obstacles < max_obs && attempts < max_obs * 20) {
        Position pos;
        pos.x = rand() % game->width;
        pos.y = rand() % game->height;
        int dup = 0;
        for (int i = 0; i < game->num_obstacles; i++) {
            if (game->obstacles[i].pos.x == pos.x && game->obstacles[i].pos.y == pos.y) {
                dup = 1;
                break;
            }
        }
        if (!dup) {
            game->obstacles[game->num_obstacles].pos = pos;
            game->num_obstacles++;
        }
        attempts++;
    }
}

void load_obstacles_from_file(Game *game, const char *filename) {
    FILE *f = fopen(filename, "r");
    if (!f) return;
    free(game->obstacles);
    game->obstacles = NULL;
    game->num_obstacles = 0;
    Obstacle *tmp = NULL;
    int count = 0;
    int x, y;
    while (fscanf(f, "%d %d", &x, &y) == 2) {
        tmp = realloc(tmp, (count + 1) * sizeof(Obstacle));
        tmp[count].pos.x = x;
        tmp[count].pos.y = y;
        count++;
    }
    fclose(f);
    game->obstacles = tmp;
    game->num_obstacles = count;
    game->has_obstacles = (count > 0);
}

void update_game(Game *game) {
    if (game->game_over) return;
    if (game->paused) return;

    int now = (int)time(NULL);

    if (game->game_mode == 1 && game->time_limit > 0) {
        if (now - game->start_time >= game->time_limit) {
            game->game_over = true;
            return;
        }
    }

    if (game->freeze_until > now) {
        return;
    }

    for (int i = 0; i < game->num_snakes; i++) {
        Snake *s = &game->snakes[i];
        if (s->length == 0) continue;
        if (s->sleep_until > now) {
            continue;
        }

        Position tail_before = s->body[s->length - 1];
        move_snake(s);

        if (!game->has_obstacles) {
            if (s->body[0].x < 0) s->body[0].x = game->width - 1;
            else if (s->body[0].x >= game->width) s->body[0].x = 0;
            if (s->body[0].y < 0) s->body[0].y = game->height - 1;
            else if (s->body[0].y >= game->height) s->body[0].y = 0;
        }

        if (is_collision(game, s)) {
            game->scores[i] = s->length - 1;
            remove_snake(game, i);
            continue;
        }

        Position head = s->body[0];
        for (int j = 0; j < game->num_fruits; j++) {
            if (head.x == game->fruits[j].pos.x && head.y == game->fruits[j].pos.y) {
                s->length++;
                s->body = realloc(s->body, s->length * sizeof(Position));
                s->body[s->length - 1] = tail_before;
                game->scores[i] = s->length - 1;
                for (int k = j; k < game->num_fruits - 1; k++) {
                    game->fruits[k] = game->fruits[k + 1];
                }
                game->num_fruits--;
                if (game->num_fruits > 0) {
                    game->fruits = realloc(game->fruits, game->num_fruits * sizeof(Fruit));
                } else {
                    free(game->fruits);
                    game->fruits = NULL;
                }
                generate_fruit(game);
                break;
            }
        }
    }

    if (game->num_snakes == 0) {
        if (game->empty_since == 0) {
            game->empty_since = now;
        } else if (game->game_mode == 0 && now - game->empty_since >= 10) {
            game->game_over = true;
        }
    } else {
        int alive = 0;
        for (int i = 0; i < game->num_snakes; i++) {
            if (game->snakes[i].length > 0) alive++;
        }
        
        if (game->num_snakes > 1 && alive == 1) {
            game->game_over = true;
        }
        
        if (game->num_snakes == 1 && alive == 0) {
            game->game_over = true;
        }
        
        if (alive == 0 && game->num_snakes > 1) {
            if (game->empty_since == 0) {
                game->empty_since = now;
            } else if (game->game_mode == 0 && now - game->empty_since >= 10) {
                game->game_over = true;
            }
        } else {
            game->empty_since = 0;
        }
    }
}

bool is_collision(Game *game, Snake *snake) {
    Position head = snake->body[0];
    if (game->has_obstacles) {
        if (head.x < 0 || head.x >= game->width || head.y < 0 || head.y >= game->height) {
            return true;
        }
    }
    for (int i = 1; i < snake->length; i++) {
        if (head.x == snake->body[i].x && head.y == snake->body[i].y) {
            return true;
        }
    }
    for (int i = 0; i < game->num_snakes; i++) {
        if (&game->snakes[i] != snake && game->snakes[i].length > 0) {
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
    int alive = 0;
    for (int i = 0; i < game->num_snakes; i++) {
        if (game->snakes[i].length > 0) alive++;
    }
    if (alive == 0) return;
    if (game->num_fruits >= alive) return;
    Position pos;
    int attempts = 0;
    while (attempts < 200) {
        pos.x = rand() % game->width;
        pos.y = rand() % game->height;
        int bad = 0;
        for (int i = 0; i < game->num_snakes; i++) {
            if (game->snakes[i].length == 0) continue;
            for (int j = 0; j < game->snakes[i].length; j++) {
                if (game->snakes[i].body[j].x == pos.x && game->snakes[i].body[j].y == pos.y) { bad = 1; break; }
            }
            if (bad) break;
        }
        if (!bad) {
            for (int i = 0; i < game->num_obstacles; i++) {
                if (game->obstacles[i].pos.x == pos.x && game->obstacles[i].pos.y == pos.y) { bad = 1; break; }
            }
        }
        if (!bad) {
            for (int i = 0; i < game->num_fruits; i++) {
                if (game->fruits[i].pos.x == pos.x && game->fruits[i].pos.y == pos.y) { bad = 1; break; }
            }
        }
        if (!bad) break;
        attempts++;
    }
    if (attempts < 200) {
        game->fruits = realloc(game->fruits, (game->num_fruits + 1) * sizeof(Fruit));
        game->fruits[game->num_fruits].pos = pos;
        game->num_fruits++;
    }
}

void move_snake(Snake *snake) {
    for (int i = snake->length - 1; i > 0; i--) {
        snake->body[i] = snake->body[i - 1];
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
    snake->direction = 1;
    snake->sleep_until = 0;
    game->num_snakes++;
    game->empty_since = 0;
    generate_fruit(game);
}

void revive_snake(Game *game, int index, int start_x, int start_y) {
    if (index < 0 || index >= game->num_snakes) return;
    Snake *snake = &game->snakes[index];
    snake->length = 1;
    snake->body = malloc(sizeof(Position));
    snake->body[0].x = start_x;
    snake->body[0].y = start_y;
    snake->direction = 1;
    snake->sleep_until = 0;
    game->scores[index] = 0;
    generate_fruit(game);
}

void remove_snake(Game *game, int index) {
    if (index < 0 || index >= game->num_snakes) return;
    if (game->snakes[index].length == 0) return;
    free(game->snakes[index].body);
    game->snakes[index].body = NULL;
    game->snakes[index].length = 0;
}

char* serialize_game_state(Game *game) {
    char buffer[131072];
    buffer[0] = '\0';
    sprintf(buffer + strlen(buffer), "%d,%d;", game->width, game->height);
    for (int i = 0; i < game->num_snakes; i++) {
        Snake *s = &game->snakes[i];
        if (s->length == 0) {
            sprintf(buffer + strlen(buffer), "s0,0;");
            continue;
        }
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
    int elapsed = (int)time(NULL) - game->start_time;
    sprintf(buffer + strlen(buffer), "t%d;", elapsed);
    sprintf(buffer + strlen(buffer), "go%d;", game->game_over ? 1 : 0);
    sprintf(buffer + strlen(buffer), "p%d;", game->paused ? 1 : 0);
    return strdup(buffer);
}

