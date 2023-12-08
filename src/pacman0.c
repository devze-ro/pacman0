#include <stdio.h>
#include <stdlib.h>

#include "defines.h"
#include "raylib.h"

#define TILE_WIDTH 8
#define TILE_HEIGHT 8
#define SCREEN_WIDTH_IN_TILES 28
#define SCREEN_HEIGHT_IN_TILES 36
#define SCORE_TILE_ROW_COUNT 3
#define LEVEL_TILE_ROW_COUNT 2
#define SCALE 2.5f
#define FPS 60
#define PACMAN_SPEED 50
#define PACMAN_FRAME_COUNT_PER_ANIM_FRAME 4
#define PACMAN_MOVE_ANIM_FRAME_COUNT 4
#define PACMAN_CORNERING_RANGE 3.5f
#define GHOST_SPEED 40
#define GHOST_FRAME_COUNT_PER_ANIM_FRAME 8
#define GHOST_MOVE_ANIM_FRAME_COUNT 2

global v2i dir_vectors[4] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};

typedef enum Direction { LEFT, RIGHT, UP, DOWN } Direction;

typedef struct PacMan {
    v2 pos;
    v2 half_dim;
    v2 vel;
    Direction dir;
    Rectangle idle_anim_frames[1];
    Rectangle left_anim_frames[4];
    Rectangle right_anim_frames[4];
    Rectangle up_anim_frames[4];
    Rectangle down_anim_frames[4];
} PacMan;

internal f32 fabs(f32 x) { return x < 0 ? x * -1 : x; }

internal v2 v2_add(v2 vec1, v2 vec2) {
    return (v2){vec1.x + vec2.x, vec1.y + vec2.y};
}
internal v2u v2u_add(v2u vec1, v2u vec2) {
    return (v2u){vec1.x + vec2.x, vec1.y + vec2.y};
}
internal v2i v2i_add(v2i vec1, v2i vec2) {
    return (v2i){vec1.x + vec2.x, vec1.y + vec2.y};
}

internal v2 v2_sub(v2 vec1, v2 vec2) {
    return (v2){vec1.x - vec2.x, vec1.y - vec2.y};
}
internal v2u v2u_sub(v2u vec1, v2u vec2) {
    return (v2u){vec1.x - vec2.x, vec1.y - vec2.y};
}
internal v2i v2i_sub(v2i vec1, v2i vec2) {
    return (v2i){vec1.x - vec2.x, vec1.y - vec2.y};
}

#define ANSI_RED "\x1b[31m"
#define ANSI_GREEN "\x1b[32m"
#define ANSI_YELLOW "\x1b[33m"
#define ANSI_BLUE "\x1b[34m"
#define ANSI_MAGENTA "\x1b[35m"
#define ANSI_CYAN "\x1b[36m"
#define ANSI_RESET "\x1b[0m"

void trace_log_callback(int log_type, const char *text, va_list args) {
    char message[256];
    vsprintf_s(message, sizeof(message), text, args);

    switch (log_type) {
        case LOG_INFO:
            printf(ANSI_CYAN "[INFO]" ANSI_RESET " %s\n", message);
            break;
        case LOG_WARNING:
            printf(ANSI_YELLOW "[WARNING]" ANSI_RESET " %s\n", message);
            break;
        case LOG_ERROR:
            printf(ANSI_RED "[ERROR]" ANSI_RESET " %s\n", message);
            break;
        case LOG_DEBUG:
            printf(ANSI_GREEN "[DEBUG]" ANSI_RESET " %s\n", message);
            break;
        // Add other cases as needed
        default:
            printf("%s\n", message);
            break;
    }
}

internal u32 **get_tile_map() {
    u32 **result = (u32 **)MemAlloc(SCREEN_HEIGHT_IN_TILES * sizeof(u32 *));
    for (i32 y = 0; y < SCREEN_HEIGHT_IN_TILES; y++) {
        result[y] = (u32 *)MemAlloc(SCREEN_WIDTH_IN_TILES * sizeof(u32));
    }
    u32 tile_map[SCREEN_HEIGHT_IN_TILES][SCREEN_WIDTH_IN_TILES] = {
        // 0th row
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
         0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        // 1st row
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
         0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        // 2nd row
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
         0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        // 3rd row
        {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
         1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
        // 4th row
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
         1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        // 5th row
        {1, 0, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 0, 1,
         1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 0, 1},
        // 6th row
        {1, 0, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 0, 1,
         1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 0, 1},
        // 7th row
        {1, 0, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 0, 1,
         1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 0, 1},
        // 8th row
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
         0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        // 9th row
        {1, 0, 1, 1, 1, 1, 0, 1, 1, 0, 1, 1, 1, 1,
         1, 1, 1, 1, 0, 1, 1, 0, 1, 1, 1, 1, 0, 1},
        // 10th row
        {1, 0, 1, 1, 1, 1, 0, 1, 1, 0, 1, 1, 1, 1,
         1, 1, 1, 1, 0, 1, 1, 0, 1, 1, 1, 1, 0, 1},
        // 11th row
        {1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 1,
         1, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1},
        // 12th row
        {1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 0, 1,
         1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1},
        // 13th row
        {0, 0, 0, 0, 0, 1, 0, 1, 1, 1, 1, 1, 0, 1,
         1, 0, 1, 1, 1, 1, 1, 0, 1, 0, 0, 0, 0, 0},
        // 14th row
        {0, 0, 0, 0, 0, 1, 0, 1, 1, 0, 0, 0, 0, 0,
         0, 0, 0, 0, 0, 1, 1, 0, 1, 0, 0, 0, 0, 0},
        // 15th row
        {0, 0, 0, 0, 0, 1, 0, 1, 1, 0, 1, 1, 1, 1,
         1, 1, 1, 1, 0, 1, 1, 0, 1, 0, 0, 0, 0, 0},
        // 16th row
        {1, 1, 1, 1, 1, 1, 0, 1, 1, 0, 1, 1, 1, 1,
         1, 1, 1, 1, 0, 1, 1, 0, 1, 1, 1, 1, 1, 1},
        // 17th row
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1,
         1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        // 18th row
        {1, 1, 1, 1, 1, 1, 0, 1, 1, 0, 1, 1, 1, 1,
         1, 1, 1, 1, 0, 1, 1, 0, 1, 1, 1, 1, 1, 1},
        // 19th row
        {0, 0, 0, 0, 0, 1, 0, 1, 1, 0, 1, 1, 1, 1,
         1, 1, 1, 1, 0, 1, 1, 0, 1, 0, 0, 0, 0, 0},
        // 20th row
        {0, 0, 0, 0, 0, 1, 0, 1, 1, 0, 0, 0, 0, 0,
         0, 0, 0, 0, 0, 1, 1, 0, 1, 0, 0, 0, 0, 0},
        // 21st row
        {0, 0, 0, 0, 0, 1, 0, 1, 1, 0, 1, 1, 1, 1,
         1, 1, 1, 1, 0, 1, 1, 0, 1, 0, 0, 0, 0, 0},
        // 22nd row
        {1, 1, 1, 1, 1, 1, 0, 1, 1, 0, 1, 1, 1, 1,
         1, 1, 1, 1, 0, 1, 1, 0, 1, 1, 1, 1, 1, 1},
        // 23rd row
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
         1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        // 24th row
        {1, 0, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 0, 1,
         1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 0, 1},
        // 25th row
        {1, 0, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 0, 1,
         1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 0, 1},
        // 26th row
        {1, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0,
         0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 1},
        // 27th row
        {1, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 1, 1,
         1, 1, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 1},
        // 28th row
        {1, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 1, 1,
         1, 1, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 1},
        // 29th row
        {1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 1,
         1, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1},
        // 30th row
        {1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1,
         1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1},
        // 31st row
        {1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1,
         1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1},
        // 32nd row
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
         0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        // 33rd row
        {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
         1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
        // 34th row
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
         0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        // 35th row
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
         0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    };

    for (i32 y = 0; y < SCREEN_HEIGHT_IN_TILES; y++) {
        for (i32 x = 0; x < SCREEN_WIDTH_IN_TILES; x++) {
            result[y][x] = tile_map[y][x];
        }
    }
    return result;
}

internal void init_pacman(PacMan *pacman) {
    pacman->pos = (v2){112, 212};
    pacman->half_dim = (v2){TILE_WIDTH, TILE_HEIGHT};
    pacman->vel = (v2){PACMAN_SPEED, PACMAN_SPEED};
    pacman->dir = LEFT;

    f32 width_with_border = pacman->half_dim.x * 2;
    f32 height_with_border = pacman->half_dim.y * 2;
    f32 w = width_with_border - 2;
    f32 h = height_with_border - 2;

    pacman->idle_anim_frames[0] =
        (Rectangle){width_with_border * 2 + 1, 1, w, h};

    pacman->left_anim_frames[0] =
        (Rectangle){width_with_border * 2 + 1, 1, w, h};
    pacman->left_anim_frames[1] =
        (Rectangle){width_with_border + 1, height_with_border + 1, w, h};
    pacman->left_anim_frames[2] =
        (Rectangle){0 + 1, height_with_border + 1, w, h};
    pacman->left_anim_frames[3] =
        (Rectangle){width_with_border + 1, height_with_border + 1, w, h};

    pacman->right_anim_frames[0] =
        (Rectangle){width_with_border * 2 + 1, 1, w, h};
    pacman->right_anim_frames[1] =
        (Rectangle){width_with_border + 1, 1, w, h};
    pacman->right_anim_frames[2] = (Rectangle){1, 1, w, h};
    pacman->right_anim_frames[3] =
        (Rectangle){width_with_border + 1, 1, w, h};

    pacman->up_anim_frames[0] =
        (Rectangle){width_with_border * 2 + 1, 1, w, h};
    pacman->up_anim_frames[1] =
        (Rectangle){width_with_border + 1, height_with_border * 2 + 1, w, h};
    pacman->up_anim_frames[2] =
        (Rectangle){0 + 1, height_with_border * 2 + 1, w, h};
    pacman->up_anim_frames[3] =
        (Rectangle){width_with_border + 1, height_with_border * 2 + 1, w, h};

    pacman->down_anim_frames[0] =
        (Rectangle){width_with_border * 2 + 1, 1, w, h};
    pacman->down_anim_frames[1] =
        (Rectangle){width_with_border + 1, height_with_border * 3 + 1, w, h};
    pacman->down_anim_frames[2] =
        (Rectangle){0 + 1, height_with_border * 3 + 1, w, h};
    pacman->down_anim_frames[3] =
        (Rectangle){width_with_border + 1, height_with_border * 3 + 1, w, h};
}

internal v2 get_next_pos(v2 *curr_pos, v2 *vel, v2i *dir_vec, f32 dt) {
    v2 pos_change = (v2){vel->x * dir_vec->x * dt, vel->y * dir_vec->y * dt};
    v2 next_pos = v2_add(*curr_pos, pos_change);
    return next_pos;
}

internal b32 can_move(u32 **tile_map, v2 *next_pos, v2i *curr_tile,
                      v2 *curr_tile_pos, v2i *dir_vec, b32 is_dir_same) {
    v2i next_tile = v2i_add(*curr_tile, *dir_vec);
    TraceLog(LOG_DEBUG, "Next tile: [%d][%d]\n", next_tile.x, next_tile.y);

    if (next_tile.x < 0 || next_tile.x >= SCREEN_WIDTH_IN_TILES) {
        return 1;
    }

    v2 dist_to_tile_mid = v2_sub(*next_pos, *curr_tile_pos);
    b32 result = 0;
    b32 is_tile_occupied = tile_map[next_tile.y][next_tile.x] == 1 ? 1 : 0;
    b32 can_corner = 0;

    if (dir_vec->x != 0) {
        can_corner = fabs(dist_to_tile_mid.y) <= PACMAN_CORNERING_RANGE;
        result = (is_dir_same && ((dist_to_tile_mid.x * dir_vec->x) <= 0 ||
                                  !is_tile_occupied)) ||
                         (!is_dir_same && !is_tile_occupied && can_corner)
                     ? 1
                     : 0;
    }

    if (dir_vec->y != 0) {
        can_corner = fabs(dist_to_tile_mid.x) <= PACMAN_CORNERING_RANGE;
        result = (is_dir_same && ((dist_to_tile_mid.y * dir_vec->y) <= 0 ||
                                  !is_tile_occupied)) ||
                         (!is_dir_same && !is_tile_occupied && can_corner)
                     ? 1
                     : 0;
    }

    return result;
}

internal void move(v2 *curr_pos, v2 *curr_tile_pos, v2 *next_pos, v2i *dir_vec,
                   b32 is_dir_same) {
    if (!is_dir_same) {
        TraceLog(LOG_DEBUG, "Pac-Man direction changed.\n");
        v2 dist_to_tile_mid = v2_sub(*next_pos, *curr_tile_pos);

        if (dir_vec->x != 0) {
            next_pos->y -= dist_to_tile_mid.y;
        }
        if (dir_vec->y != 0) {
            next_pos->x -= dist_to_tile_mid.x;
        }
    }
    curr_pos->x = next_pos->x;
    curr_pos->y = next_pos->y;
}

internal void resolve_wall_collision(v2 *next_pos, v2 *curr_tile_pos,
                                     v2i *dir_vec) {
    TraceLog(LOG_DEBUG, "========== COLLISION RESOLUTION ==========\n");
    v2 dist_to_tile_mid = v2_sub(*next_pos, *curr_tile_pos);

    if (dir_vec->x != 0) {
        next_pos->x -= dist_to_tile_mid.x;
    }
    if (dir_vec->y != 0) {
        next_pos->y -= dist_to_tile_mid.y;
    }
}

i32 main(void) {
    SetTraceLogCallback(trace_log_callback);
    SetTraceLogLevel(LOG_DEBUG);

    f32 tile_width_inv = 1 / (f32)TILE_WIDTH;
    f32 tile_height_inv = 1 / (f32)TILE_HEIGHT;

    u32 back_buffer_width = TILE_WIDTH * SCREEN_WIDTH_IN_TILES;
    u32 back_buffer_height = TILE_HEIGHT * SCREEN_HEIGHT_IN_TILES;
    u32 screen_width = (u32)(back_buffer_width * SCALE);
    u32 screen_height = (u32)(back_buffer_height * SCALE);

    InitWindow(screen_width, screen_height, "pacman0");
    SetTargetFPS(FPS);

    Texture2D sprite_tex = LoadTexture("assets/sprite.png");
    Texture2D maze_tex = LoadTexture("assets/blue-maze.png");
    RenderTexture2D back_buffer =
        LoadRenderTexture(back_buffer_width, back_buffer_height);

    v2u maze_start_corner = {0, SCORE_TILE_ROW_COUNT * TILE_HEIGHT};
    PacMan pacman = {0};
    init_pacman(&pacman);

    u32 **tile_map = get_tile_map();
    Rectangle *pacman_anim_current_frames = pacman.left_anim_frames;
    u32 pacman_anim_frame_index = 0;
    u32 pacman_frame_counter = 0;

    while (!WindowShouldClose()) {
        TraceLog(LOG_DEBUG, "Pac-Man position BEFORE update: [%3.2f][%3.2f]\n",
                 pacman.pos.x, pacman.pos.y);
        f32 dt = GetFrameTime();
        u32 pacman_anim_frame_count = PACMAN_MOVE_ANIM_FRAME_COUNT;

        v2i pacman_curr_tile = {(i32)(pacman.pos.x * tile_width_inv),
                                (i32)(pacman.pos.y * tile_height_inv)};
        v2 pacman_curr_tile_pos = {
            (pacman_curr_tile.x + 0.5f) * (f32)TILE_WIDTH,
            (pacman_curr_tile.y + 0.5f) * (f32)TILE_HEIGHT};

        Direction pacman_next_dir = pacman.dir;
        if (IsKeyDown(KEY_LEFT)) {
            pacman_next_dir = LEFT;
        }
        if (IsKeyDown(KEY_RIGHT)) {
            pacman_next_dir = RIGHT;
        }
        if (IsKeyDown(KEY_UP)) {
            pacman_next_dir = UP;
        }
        if (IsKeyDown(KEY_DOWN)) {
            pacman_next_dir = DOWN;
        }

        TraceLog(LOG_DEBUG, "Pacman tile: [%d][%d]\n", pacman_curr_tile.x,
                 pacman_curr_tile.y);
        b32 is_pacman_idle = 0;
        b32 is_dir_same = pacman_next_dir == pacman.dir ? 1 : 0;
        v2i pacman_next_dir_vec = dir_vectors[pacman_next_dir];
        v2 pacman_next_pos =
            get_next_pos(&pacman.pos, &pacman.vel, &pacman_next_dir_vec, dt);

        b32 can_pacman_move = 0;

        // update check
        if (pacman_next_pos.x < 0) {
            pacman_next_pos.x = (f32)(back_buffer_width - 1);
            can_pacman_move = 1;
        } else if (pacman_next_pos.x >= back_buffer_width) {
            pacman_next_pos.x = 0;
            can_pacman_move = 1;
        } else {
            TraceLog(LOG_DEBUG,
                     "========== CHECKING NEXT DIRECTION ==========\n");
            can_pacman_move = can_move(tile_map, &pacman_next_pos,
                                       &pacman_curr_tile, &pacman_curr_tile_pos,
                                       &pacman_next_dir_vec, is_dir_same);

            TraceLog(LOG_DEBUG, "Can Pac-Man move in next dir: %d\n",
                     can_pacman_move);
            if (can_pacman_move) {
                pacman.dir = pacman_next_dir;
            } else {
                TraceLog(LOG_DEBUG,
                         "========== CHECKING CURR DIRECTION ==========\n");
                is_dir_same = 1;
                pacman_next_dir_vec = dir_vectors[pacman.dir];
                pacman_next_pos = get_next_pos(&pacman.pos, &pacman.vel,
                                               &pacman_next_dir_vec, dt);
                TraceLog(LOG_DEBUG,
                         "Pac-Man next pos in curr dir: [%3.2f][%3.2f]\n",
                         pacman_next_pos.x, pacman_next_pos.y);
                can_pacman_move = can_move(
                    tile_map, &pacman_next_pos, &pacman_curr_tile,
                    &pacman_curr_tile_pos, &pacman_next_dir_vec, is_dir_same);
                TraceLog(LOG_DEBUG, "Can Pac-Man move in curr dir: %d\n",
                         can_pacman_move);
            }
        }

        // update
        if (can_pacman_move) {
            TraceLog(LOG_DEBUG, "Pac-Man can move.\n");
            move(&pacman.pos, &pacman_curr_tile_pos, &pacman_next_pos,
                 &pacman_next_dir_vec, is_dir_same);
        } else if (is_dir_same) {
            TraceLog(LOG_DEBUG, "========== CURR DIR COLLISION ==========\n");
            resolve_wall_collision(&pacman_next_pos, &pacman_curr_tile_pos,
                                   &pacman_next_dir_vec);
            is_pacman_idle = 1;
            pacman_anim_current_frames = pacman.idle_anim_frames;
        }

        // animate
        if (!is_pacman_idle) {
            switch (pacman.dir) {
                case LEFT:
                    pacman_anim_current_frames = pacman.left_anim_frames;
                    break;
                case RIGHT:
                    pacman_anim_current_frames = pacman.right_anim_frames;
                    break;
                case UP:
                    pacman_anim_current_frames = pacman.up_anim_frames;
                    break;
                case DOWN:
                    pacman_anim_current_frames = pacman.down_anim_frames;
                    break;
            }
        }

        pacman_frame_counter++;
        if (pacman_frame_counter >= PACMAN_FRAME_COUNT_PER_ANIM_FRAME) {
            pacman_anim_frame_index++;
            pacman_frame_counter = 0;
        }
        if (pacman_anim_frame_index >= pacman_anim_frame_count) {
            pacman_anim_frame_index = 0;
        }

        BeginTextureMode(back_buffer);
        {
            ClearBackground(BLACK);
            DrawTexture(maze_tex, maze_start_corner.x, maze_start_corner.y,
                        WHITE);
            DrawTextureRec(sprite_tex,
                           pacman_anim_current_frames[pacman_anim_frame_index],
                           // NOTE: Added 1 to account for the 1 pixel padding
                           // around the images.
                           (v2){pacman.pos.x + 1 - pacman.half_dim.x,
                                pacman.pos.y + 1 - pacman.half_dim.y},
                           WHITE);
        }
        EndTextureMode();

        BeginDrawing();
        {
            DrawTexturePro(
                back_buffer.texture,
                (Rectangle){0, 0, (f32)(back_buffer.texture.width),
                            (f32)(-back_buffer.texture.height)},
                (Rectangle){0, 0, (f32)(screen_width), (f32)(screen_height)},
                (v2){0, 0}, 0.0f, WHITE);
        }
        EndDrawing();
        TraceLog(LOG_DEBUG, "Pac-Man pos AFTER update: [%3.2f][%3.2f]\n\n\n",
                 pacman.pos.x, pacman.pos.y);
    }
    CloseWindow();
    return 0;
}
