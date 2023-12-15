#include <stdio.h>
#include <stdlib.h>

#include "defines.h"
#include "raylib.h"

#define TILE_WIDTH 8
#define TILE_WIDTH_INV 1 / (f32)TILE_WIDTH
#define TILE_HEIGHT 8
#define TILE_HEIGHT_INV 1 / (f32)TILE_HEIGHT
#define SCREEN_WIDTH_IN_TILES 28
#define SCREEN_HEIGHT_IN_TILES 36
#define BACK_BUFFER_WIDTH SCREEN_WIDTH_IN_TILES *TILE_WIDTH
#define BACK_BUFFER_HEIGHT SCREEN_HEIGHT_IN_TILES *TILE_HEIGHT
#define SCORE_TILE_ROW_COUNT 3
#define LEVEL_TILE_ROW_COUNT 2
#define SPRITE_TILE_WIDTH 16
#define SPRITE_TILE_HEIGHT 16
#define SPRITE_WIDTH_IN_TILES 14
#define SPRITE_HEIGHT_IN_TILES 8
#define SCALE 2.5f
#define FPS 60
#define PACMAN_SPEED 50
#define PACMAN_FRAME_COUNT_PER_ANIM_FRAME 4
#define PACMAN_MOVE_ANIM_FRAME_COUNT 4
#define PACMAN_IDLE_ANIM_FRAME_COUNT 1
#define PACMAN_DIE_ANIM_FRAME_COUNT 11
#define PACMAN_CORNERING_RANGE 3.5f
#define GHOST_CORNERING_RANGE 3.0f
#define GHOST_SPEED 50
#define GHOST_FRAME_COUNT_PER_ANIM_FRAME 8
#define GHOST_MOVE_ANIM_FRAME_COUNT 2

global v2i dir_vectors[4] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};

typedef enum Direction { DIR_LEFT, DIR_RIGHT, DIR_UP, DIR_DOWN } Direction;
typedef enum PacManState { PACMAN_IDLE, PACMAN_MOVING } PacManState;

typedef enum GhostType { BLINKY, PINKY, INKY, CLYDE } GhostType;

typedef enum GhostAnimType {
    GHOST_GOING_LEFT,
    GHOST_GOING_RIGHT,
    GHOST_GOING_UP,
    GHOST_GOING_DOWN,
    GHOST_ANIM_TYPE_COUNT
} GhostAnimType;

typedef enum PacManAnimType {
    PACMAN_IDLING,
    PACMAN_GOING_LEFT,
    PACMAN_GOING_RIGHT,
    PACMAN_GOING_UP,
    PACMAN_GOING_DOWN,
    PACMAN_DYING,
    PACMAN_ANIM_TYPE_COUNT
} PacManAnimType;

typedef struct Actor {
    v2 pos;
    v2 half_dim;
    v2 vel;
    Direction dir;
    Rectangle *anim_frames;
    u32 anim_frame_count;
    u32 anim_frame_counter;
    u32 frames_per_anim_frame;
    u32 anim_frame_index;
    f32 cornering_range;
    b32 can_turn;
} Actor;

typedef struct PacMan {
    Actor actor;
    PacManState state;
    PacManAnimType anim_type;
    u32 anim_indexes_in_sprite[PACMAN_ANIM_TYPE_COUNT];
} PacMan;

typedef struct Ghost {
    GhostType type;
    Actor actor;
    GhostAnimType anim_type;
    u32 anim_indexes_in_sprite[GHOST_ANIM_TYPE_COUNT];
} Ghost;

internal f32 fabs(f32 x) { return x < 0 ? x * -1 : x; }

internal f32 distance_sq(v2 p1, v2 p2) {
    f32 x_diff = p1.x - p2.x;
    f32 y_diff = p1.y - p2.y;
    return ((x_diff * x_diff) + (y_diff * y_diff));
}

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
internal b32 v2i_is_equal(v2i vec1, v2i vec2) {
    return vec1.x == vec2.x && vec1.y == vec2.y ? 1 : 0;
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
        default:
            printf("%s\n", message);
            break;
    }
}

internal u32 *get_tile_map() {
    u32 tile_count = SCREEN_WIDTH_IN_TILES * SCREEN_HEIGHT_IN_TILES;
    u32 *result = (u32 *)MemAlloc(tile_count * sizeof(u32));
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
            result[(y * SCREEN_WIDTH_IN_TILES) + x] = tile_map[y][x];
        }
    }
    return result;
}

internal Rectangle *get_sprite_tiles() {
    u32 tile_count = SPRITE_WIDTH_IN_TILES * SPRITE_HEIGHT_IN_TILES;
    Rectangle *result = (Rectangle *)MemAlloc(tile_count * sizeof(Rectangle));

    for (i32 y = 0; y < SPRITE_HEIGHT_IN_TILES; y++) {
        for (i32 x = 0; x < SPRITE_WIDTH_IN_TILES; x++) {
            result[(y * SPRITE_WIDTH_IN_TILES) + x] = (Rectangle){
                (f32)SPRITE_TILE_WIDTH * x + 1, (f32)SPRITE_TILE_HEIGHT * y + 1,
                SPRITE_TILE_WIDTH, SPRITE_TILE_HEIGHT};
        }
    }

    return result;
}

internal void init_pacman(PacMan *pacman, Rectangle *sprite_tiles) {
    pacman->actor = (Actor){0};
    pacman->actor.pos = (v2){112, 212};
    pacman->actor.half_dim =
        (v2){SPRITE_TILE_WIDTH * 0.5, SPRITE_TILE_HEIGHT * 0.5};
    pacman->actor.vel = (v2){PACMAN_SPEED, PACMAN_SPEED};
    pacman->actor.dir = DIR_LEFT;
    pacman->state = PACMAN_MOVING;

    pacman->anim_indexes_in_sprite[PACMAN_IDLING] = 0;
    pacman->anim_indexes_in_sprite[PACMAN_GOING_LEFT] = 4;
    pacman->anim_indexes_in_sprite[PACMAN_GOING_RIGHT] = 0;
    pacman->anim_indexes_in_sprite[PACMAN_GOING_UP] = 8;
    pacman->anim_indexes_in_sprite[PACMAN_GOING_DOWN] = 12;
    pacman->anim_type = PACMAN_GOING_LEFT;
    pacman->actor.anim_frames =
        sprite_tiles + pacman->anim_indexes_in_sprite[pacman->anim_type];
    pacman->actor.anim_frame_count = PACMAN_MOVE_ANIM_FRAME_COUNT;
    pacman->actor.anim_frame_counter = 0;
    pacman->actor.frames_per_anim_frame = PACMAN_FRAME_COUNT_PER_ANIM_FRAME;
    pacman->actor.anim_frame_index = 0;

    pacman->actor.cornering_range = PACMAN_CORNERING_RANGE;
    pacman->actor.can_turn = 1;
}

internal void init_blinky(Ghost *ghost, Rectangle *sprite_tiles) {
    ghost->type = BLINKY;
    ghost->actor = (Actor){0};
    ghost->actor.pos = (v2){112, 116};
    ghost->actor.half_dim =
        (v2){SPRITE_TILE_WIDTH * 0.5, SPRITE_TILE_HEIGHT * 0.5};
    ghost->actor.vel = (v2){GHOST_SPEED, GHOST_SPEED};
    ghost->actor.dir = DIR_LEFT;

    ghost->anim_indexes_in_sprite[GHOST_GOING_LEFT] =
        4 * SPRITE_WIDTH_IN_TILES + 2;
    ghost->anim_indexes_in_sprite[GHOST_GOING_RIGHT] =
        4 * SPRITE_WIDTH_IN_TILES + 0;
    ghost->anim_indexes_in_sprite[GHOST_GOING_UP] =
        4 * SPRITE_WIDTH_IN_TILES + 4;
    ghost->anim_indexes_in_sprite[GHOST_GOING_DOWN] =
        4 * SPRITE_WIDTH_IN_TILES + 6;
    ghost->anim_type = GHOST_GOING_LEFT;
    ghost->actor.anim_frames =
        sprite_tiles + ghost->anim_indexes_in_sprite[ghost->anim_type];
    ghost->actor.anim_frame_count = GHOST_MOVE_ANIM_FRAME_COUNT;
    ghost->actor.anim_frame_counter = 0;
    ghost->actor.frames_per_anim_frame = GHOST_FRAME_COUNT_PER_ANIM_FRAME;
    ghost->actor.anim_frame_index = 0;

    ghost->actor.cornering_range = GHOST_CORNERING_RANGE;
    ghost->actor.can_turn = 1;
}

internal v2 get_next_pos(v2 *curr_pos, v2 *vel, v2i *dir_vec, f32 dt) {
    v2 pos_change = (v2){vel->x * dir_vec->x * dt, vel->y * dir_vec->y * dt};
    v2 next_pos = v2_add(*curr_pos, pos_change);
    return next_pos;
}

internal b32 can_move(u32 *tile_map, v2 *next_pos, v2i *curr_tile,
                      v2 *curr_tile_pos, v2i *dir_vec, b32 is_dir_same) {
    v2i next_tile = v2i_add(*curr_tile, *dir_vec);
    // TraceLog(LOG_DEBUG, "Next tile: [%d][%d]\n", next_tile.x, next_tile.y);

    if (next_tile.x < 0 || next_tile.x >= SCREEN_WIDTH_IN_TILES) {
        return 1;
    }

    v2 dist_to_tile_mid = v2_sub(*next_pos, *curr_tile_pos);
    // TraceLog(LOG_DEBUG, "Tile center distance: [%d][%d]\n",
    // dist_to_tile_mid.x,
    //          dist_to_tile_mid.y);
    b32 result = 0;
    b32 is_tile_occupied =
        tile_map[(next_tile.y * SCREEN_WIDTH_IN_TILES) + next_tile.x] == 1 ? 1
                                                                           : 0;
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
        // TraceLog(LOG_DEBUG, "Pac-Man direction changed.\n");
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
    // TraceLog(LOG_DEBUG, "========== COLLISION RESOLUTION ==========\n");
    v2 dist_to_tile_mid = v2_sub(*next_pos, *curr_tile_pos);

    if (dir_vec->x != 0) {
        next_pos->x -= dist_to_tile_mid.x;
    }
    if (dir_vec->y != 0) {
        next_pos->y -= dist_to_tile_mid.y;
    }
}

internal v2i get_tile(v2 pos) {
    return (v2i){(i32)(pos.x * TILE_WIDTH_INV), (i32)(pos.y * TILE_HEIGHT_INV)};
}

internal v2 get_tile_pos(v2i tile) {
    return (v2){(f32)((tile.x + 0.5f) * TILE_WIDTH),
                (f32)((tile.y + 0.5f) * TILE_HEIGHT)};
}

internal Direction get_opposite_dir(Direction dir) {
    switch (dir) {
        case DIR_LEFT:
            return DIR_RIGHT;
        case DIR_RIGHT:
            return DIR_LEFT;
        case DIR_UP:
            return DIR_DOWN;
        case DIR_DOWN:
            return DIR_UP;
        default:
            return -1;
    }
}

typedef struct GhostTarget {
    v2 tile_pos;
    v2i closest_tile;
    f32 closest_tile_dist_sq;
    Direction closest_dir;
} GhostTarget;

internal void update_target_route(v2i *curr_tile, u32 *tile_map,
                                  Direction dir_to_check, GhostTarget *target) {
    v2i dir_vec = dir_vectors[dir_to_check];
    v2i tile_to_check = {curr_tile->x + dir_vec.x, curr_tile->y + dir_vec.y};
    TraceLog(LOG_DEBUG, "Tile to check: [%d][%d]\n", tile_to_check.x,
             tile_to_check.y);

    if (!tile_map[tile_to_check.y * SCREEN_WIDTH_IN_TILES + tile_to_check.x]) {
        v2 tile_to_check_pos = get_tile_pos(tile_to_check);
        TraceLog(LOG_DEBUG, "Tile to check pos: [%3.2f][%3.2f]\n",
                 tile_to_check_pos.x, tile_to_check_pos.y);
        f32 tile_to_check_dist_sq =
            distance_sq(target->tile_pos, tile_to_check_pos);

        if (tile_to_check_dist_sq < target->closest_tile_dist_sq ||
            target->closest_tile_dist_sq == 0) {
            target->closest_tile_dist_sq = tile_to_check_dist_sq;
            target->closest_dir = dir_to_check;
        }
    }
}

internal void animate(Actor *actor) {
    actor->anim_frame_counter++;
    if (actor->anim_frame_counter >= actor->frames_per_anim_frame) {
        actor->anim_frame_index++;
        actor->anim_frame_counter = 0;
    }
    if (actor->anim_frame_index >= actor->anim_frame_count) {
        actor->anim_frame_index = 0;
    }
}

internal void update_ghost(Ghost *ghost, v2 *target_tile_pos,
                           Rectangle *sprite_tiles, u32 *tile_map, f32 dt) {
    v2i curr_tile = get_tile(ghost->actor.pos);
    v2 curr_tile_pos = get_tile_pos(curr_tile);
    v2i dir_vec = dir_vectors[ghost->actor.dir];
    v2 dist_to_tile_mid = v2_sub(curr_tile_pos, ghost->actor.pos);
    TraceLog(LOG_DEBUG, "Direction BEFORE update: [%d]\n", ghost->actor.dir);

    b32 can_corner = 0;
    if (dir_vec.x != 0) {
        can_corner = fabs(dist_to_tile_mid.x) < ghost->actor.cornering_range;
    } else {
        can_corner = fabs(dist_to_tile_mid.y) < ghost->actor.cornering_range;
    }

    // Ensure that the ghost has moved out of the cornering region after a turn
    // else the ghost may backtrack at turns by making opposite turns in 2
    // consecutive frames.
    if (!ghost->actor.can_turn) {
        ghost->actor.can_turn = can_corner ? 0 : 1;
    }

    if (can_corner && ghost->actor.can_turn) {
        TraceLog(LOG_DEBUG, "========== GHOST CAN TURN ==========\n");
        Direction reverse_dir = get_opposite_dir(ghost->actor.dir);
        TraceLog(LOG_DEBUG, "Current dir: %d\n", ghost->actor.dir);
        TraceLog(LOG_DEBUG, "Reverse dir: %d\n", reverse_dir);
        GhostTarget ghost_target = {0};
        ghost_target.tile_pos = *target_tile_pos;
        ghost_target.closest_dir = ghost->actor.dir;

        if (reverse_dir != DIR_UP) {
            update_target_route(&curr_tile, tile_map, DIR_UP, &ghost_target);
        }
        if (reverse_dir != DIR_LEFT) {
            update_target_route(&curr_tile, tile_map, DIR_LEFT, &ghost_target);
        }
        if (reverse_dir != DIR_DOWN) {
            update_target_route(&curr_tile, tile_map, DIR_DOWN, &ghost_target);
        }
        if (reverse_dir != DIR_RIGHT) {
            update_target_route(&curr_tile, tile_map, DIR_RIGHT, &ghost_target);
        }
        TraceLog(LOG_DEBUG, "Closest dir: %d\n", ghost_target.closest_dir);
        TraceLog(LOG_DEBUG, "Closest tile dist: [%3.2f]\n",
                 ghost_target.closest_tile_dist_sq);
        if (ghost_target.closest_dir != ghost->actor.dir) {
            ghost->actor.dir = ghost_target.closest_dir;
            ghost->actor.pos = curr_tile_pos;
            ghost->actor.can_turn = 0;

            switch (ghost->actor.dir) {
                case DIR_LEFT:
                    ghost->anim_type = GHOST_GOING_LEFT;
                    break;
                case DIR_RIGHT:
                    ghost->anim_type = GHOST_GOING_RIGHT;
                    break;
                case DIR_UP:
                    ghost->anim_type = GHOST_GOING_UP;
                    break;
                case DIR_DOWN:
                    ghost->anim_type = GHOST_GOING_DOWN;
                    break;
            }
            ghost->actor.anim_frames =
                sprite_tiles +
                ghost->anim_indexes_in_sprite[ghost->anim_type];
            ghost->actor.anim_frame_count = GHOST_MOVE_ANIM_FRAME_COUNT;
        }
    }

    dir_vec = dir_vectors[ghost->actor.dir];
    TraceLog(LOG_DEBUG, "Direction AFTER update: [%d]\n", ghost->actor.dir);
    ghost->actor.pos =
        get_next_pos(&ghost->actor.pos, &ghost->actor.vel, &dir_vec, dt);
    if (ghost->actor.pos.x < 0) {
        ghost->actor.pos.x = (f32)(BACK_BUFFER_WIDTH - 1);
    } else if (ghost->actor.pos.x >= BACK_BUFFER_WIDTH) {
        ghost->actor.pos.x = 0;
    }
    TraceLog(LOG_DEBUG, "Blinky position AFTER update: [%3.2f][%3.2f]\n\n",
             ghost->actor.pos.x, ghost->actor.pos.y);
    animate(&ghost->actor);
}

internal void update_pacman(PacMan *pacman, Rectangle *sprite_tiles,
                            u32 *tile_map, f32 dt) {
    pacman->state = PACMAN_MOVING;
    v2i pacman_curr_tile = {(i32)(pacman->actor.pos.x * TILE_WIDTH_INV),
                            (i32)(pacman->actor.pos.y * TILE_HEIGHT_INV)};
    v2 pacman_curr_tile_pos = {(pacman_curr_tile.x + 0.5f) * (f32)TILE_WIDTH,
                               (pacman_curr_tile.y + 0.5f) * (f32)TILE_HEIGHT};

    Direction pacman_next_dir = pacman->actor.dir;
    if (IsKeyDown(KEY_LEFT)) {
        pacman_next_dir = DIR_LEFT;
    }
    if (IsKeyDown(KEY_RIGHT)) {
        pacman_next_dir = DIR_RIGHT;
    }
    if (IsKeyDown(KEY_UP)) {
        pacman_next_dir = DIR_UP;
    }
    if (IsKeyDown(KEY_DOWN)) {
        pacman_next_dir = DIR_DOWN;
    }

    // TraceLog(LOG_DEBUG, "Pacman tile: [%d][%d]\n", pacman_curr_tile.x,
    //          pacman_curr_tile.y);
    b32 is_dir_same = pacman_next_dir == pacman->actor.dir ? 1 : 0;
    v2i pacman_next_dir_vec = dir_vectors[pacman_next_dir];
    v2 pacman_next_pos = get_next_pos(&pacman->actor.pos, &pacman->actor.vel,
                                      &pacman_next_dir_vec, dt);

    b32 can_pacman_move = 0;

    // update check
    if (pacman_next_pos.x < 0) {
        pacman_next_pos.x = (f32)(BACK_BUFFER_WIDTH - 1);
        can_pacman_move = 1;
    } else if (pacman_next_pos.x >= BACK_BUFFER_WIDTH) {
        pacman_next_pos.x = 0;
        can_pacman_move = 1;
    } else {
        // TraceLog(LOG_DEBUG,
        //          "========== CHECKING NEXT DIRECTION ==========\n");
        can_pacman_move =
            can_move(tile_map, &pacman_next_pos, &pacman_curr_tile,
                     &pacman_curr_tile_pos, &pacman_next_dir_vec, is_dir_same);

        // TraceLog(LOG_DEBUG, "Can Pac-Man move in next dir: %d\n",
        //          can_pacman_move);
        if (can_pacman_move) {
            pacman->actor.dir = pacman_next_dir;
        } else {
            // TraceLog(LOG_DEBUG,
            //          "========== CHECKING CURR DIRECTION ==========\n");
            is_dir_same = 1;
            pacman_next_dir_vec = dir_vectors[pacman->actor.dir];
            pacman_next_pos =
                get_next_pos(&pacman->actor.pos, &pacman->actor.vel,
                             &pacman_next_dir_vec, dt);
            // TraceLog(LOG_DEBUG,
            //          "Pac-Man next pos in curr dir: [%3.2f][%3.2f]\n",
            //          pacman_next_pos.x, pacman_next_pos.y);
            can_pacman_move = can_move(tile_map, &pacman_next_pos,
                                       &pacman_curr_tile, &pacman_curr_tile_pos,
                                       &pacman_next_dir_vec, is_dir_same);
            // TraceLog(LOG_DEBUG, "Can Pac-Man move in curr dir: %d\n",
            //          can_pacman_move);
        }
    }

    // update
    if (can_pacman_move) {
        // TraceLog(LOG_DEBUG, "Pac-Man can move.\n");
        move(&pacman->actor.pos, &pacman_curr_tile_pos, &pacman_next_pos,
             &pacman_next_dir_vec, is_dir_same);
    } else if (is_dir_same) {
        // TraceLog(LOG_DEBUG, "========== CURR DIR COLLISION
        // ==========\n");
        resolve_wall_collision(&pacman_next_pos, &pacman_curr_tile_pos,
                               &pacman_next_dir_vec);
        pacman->state = PACMAN_IDLE;
        pacman->anim_type = PACMAN_IDLING;
        pacman->actor.anim_frames =
            sprite_tiles + pacman->anim_indexes_in_sprite[pacman->anim_type];
        pacman->actor.anim_frame_count = PACMAN_IDLE_ANIM_FRAME_COUNT;
    }

    // animate
    if (pacman->state != PACMAN_IDLE) {
        switch (pacman->actor.dir) {
            case DIR_LEFT:
                pacman->anim_type = PACMAN_GOING_LEFT;
                break;
            case DIR_RIGHT:
                pacman->anim_type = PACMAN_GOING_RIGHT;
                break;
            case DIR_UP:
                pacman->anim_type = PACMAN_GOING_UP;
                break;
            case DIR_DOWN:
                pacman->anim_type = PACMAN_GOING_DOWN;
                break;
        }

        pacman->actor.anim_frames =
            sprite_tiles +
            pacman->anim_indexes_in_sprite[pacman->anim_type];
        pacman->actor.anim_frame_count = PACMAN_MOVE_ANIM_FRAME_COUNT;
    }

    animate(&pacman->actor);
}

i32 main(void) {
    SetTraceLogCallback(trace_log_callback);
    SetTraceLogLevel(LOG_DEBUG);

    u32 screen_width = (u32)(BACK_BUFFER_WIDTH * SCALE);
    u32 screen_height = (u32)(BACK_BUFFER_HEIGHT * SCALE);

    InitWindow(screen_width, screen_height, "pacman0");
    SetTargetFPS(FPS);

    Texture2D sprite_tex = LoadTexture("assets/sprite.png");
    Texture2D maze_tex = LoadTexture("assets/blue-maze.png");
    RenderTexture2D back_buffer =
        LoadRenderTexture(BACK_BUFFER_WIDTH, BACK_BUFFER_HEIGHT);

    v2u maze_start_corner = {0, SCORE_TILE_ROW_COUNT * TILE_HEIGHT};
    u32 *tile_map = get_tile_map();
    Rectangle *sprite_tiles = get_sprite_tiles();

    PacMan pacman = {0};
    init_pacman(&pacman, sprite_tiles);
    Ghost blinky = {0};
    init_blinky(&blinky, sprite_tiles);

    while (!WindowShouldClose()) {
        // TraceLog(LOG_DEBUG, "Pac-Man position BEFORE update:
        // [%3.2f][%3.2f]\n",
        //          pacman.pos.x, pacman.pos.y);
        TraceLog(LOG_DEBUG, "Blinky position BEFORE update: [%3.2f][%3.2f]\n",
                 blinky.actor.pos.x, blinky.actor.pos.y);
        f32 dt = GetFrameTime();

        update_pacman(&pacman, sprite_tiles, tile_map, dt);
        update_ghost(&blinky, &pacman.actor.pos, sprite_tiles, tile_map, dt);

        BeginTextureMode(back_buffer);
        {
            ClearBackground(BLACK);
            DrawTexture(maze_tex, maze_start_corner.x, maze_start_corner.y,
                        WHITE);
            DrawTextureRec(
                sprite_tex,
                pacman.actor.anim_frames[pacman.actor.anim_frame_index],
                (v2){pacman.actor.pos.x - pacman.actor.half_dim.x + 1,
                     pacman.actor.pos.y - pacman.actor.half_dim.y + 1},
                WHITE);
            DrawTextureRec(
                sprite_tex,
                blinky.actor.anim_frames[blinky.actor.anim_frame_index],
                (v2){blinky.actor.pos.x - blinky.actor.half_dim.x + 1,
                     blinky.actor.pos.y - blinky.actor.half_dim.y + 1},
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
        // TraceLog(LOG_DEBUG, "Pac-Man pos AFTER update: [%3.2f][%3.2f]\n\n\n",
        //          pacman.pos.x, pacman.pos.y);
    }
    CloseWindow();
    return 0;
}
