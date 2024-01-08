#include <stdio.h>
#include <stdlib.h>

#include "defines.h"
#include "raylib.h"

#define TILE_WIDTH 8
#define TILE_WIDTH_INV (1 / (f32)TILE_WIDTH)
#define TILE_HEIGHT 8
#define TILE_HEIGHT_INV (1 / (f32)TILE_HEIGHT)
#define SCREEN_TILES_X 30
#define SCREEN_TILES_Y 38
#define BACK_BUFFER_WIDTH (SCREEN_TILES_X * TILE_WIDTH)
#define BACK_BUFFER_HEIGHT (SCREEN_TILES_Y * TILE_HEIGHT)
#define SCORE_TILE_ROW_COUNT 4
#define LEVEL_TILE_ROW_COUNT 3
#define SPRITE_TILE_WIDTH 16
#define SPRITE_TILE_HEIGHT 16
#define SPRITE_TILES_X 14
#define SPRITE_TILES_Y 8
#define SCALE 2.5f
#define DISABLED_TICK 0xFFFFFFFF
#define FPS 60
#define PACMAN_SPEED 50
#define PACMAN_TICKS_PER_ANIM_FRAME 4
#define PACMAN_IDLE_ANIM_FRAME_COUNT 1
#define PACMAN_MOVE_ANIM_FRAME_COUNT 4
#define PACMAN_DIE_ANIM_FRAME_COUNT 11
#define PACMAN_CORNERING_RANGE 3.5f
#define GHOST_CORNERING_RANGE 2.5f
#define COLLISION_RANGE 3.5f
#define GHOST_SPEED 48
#define GHOST_FRIGHTENED_SPEED 24
#define GHOST_EYES_SPEED 80
#define GHOST_TICKS_PER_ANIM_FRAME 8
#define GHOST_MOVE_ANIM_FRAME_COUNT 2
#define GHOST_EYES_ANIM_FRAME_COUNT 1
#define GHOST_PANIC_ANIM_FRAME_COUNT 2
#define GHOST_RECOVER_ANIM_FRAME_COUNT 4
#define DOT_COUNT 244
#define PILL_COUNT 4
#define DOOR_ENTRY_X (15 * TILE_WIDTH)
#define DOOR_ENTRY_Y ((15 + 0.5) * TILE_HEIGHT)
#define GHOST_HOME_CENTER_X (15 * TILE_WIDTH)
#define GHOST_HOME_CENTER_Y ((18 + 0.5) * TILE_HEIGHT)

typedef enum { DIR_UP, DIR_LEFT, DIR_DOWN, DIR_RIGHT, DIR_COUNT } Direction;

typedef enum { TILE_EMPTY, TILE_WALL, TILE_DOT, TILE_PILL, TILE_DOOR } TileType;

typedef enum {
    GHOST_BLINKY,
    GHOST_PINKY,
    GHOST_INKY,
    GHOST_CLYDE,
    GHOST_TYPE_COUNT
} GhostType;

typedef enum {
    PACMAN_IDLE,
    PACMAN_MOVING,
    PACMAN_CAUGHT,
    PACMAN_DEAD
} PacManState;

typedef enum {
    GHOST_SCATTER,
    GHOST_CHASE,
    GHOST_PANIC,
    GHOST_RECOVER,
    GHOST_EATEN,
    GHOST_EYES,
    GHOST_ENTER_HOME,
    GHOST_HOME,
    GHOST_LEAVE_HOME
} GhostState;

typedef enum {
    GAME_INTRO,
    GAME_PRELUDE,
    GAME_READY,
    GAME_IN_PROGRESS,
    GAME_PAUSE,
    GAME_LEVEL_COMPLETE,
    GAME_ROUND_OVER,
    GAME_OVER
} GameState;

typedef enum {
    GHOST_GOING_LEFT,
    GHOST_GOING_RIGHT,
    GHOST_GOING_UP,
    GHOST_GOING_DOWN,
    GHOST_PANICKING,
    GHOST_RECOVERING,
    GHOST_EYES_LEFT,
    GHOST_EYES_RIGHT,
    GHOST_EYES_UP,
    GHOST_EYES_DOWN,
    GHOST_ANIM_TYPE_COUNT
} GhostAnimType;

typedef enum {
    PACMAN_IDLING,
    PACMAN_GOING_LEFT,
    PACMAN_GOING_RIGHT,
    PACMAN_GOING_UP,
    PACMAN_GOING_DOWN,
    PACMAN_DYING,
    PACMAN_ANIM_TYPE_COUNT
} PacManAnimType;

typedef struct {
    u32 tick;
} Event;

typedef struct {
    Rectangle *frames;
    u32 frame_count;
    u32 frame_counter;
    u32 ticks_per_anim_frame;
    u32 frame_index;
} Animation;

typedef struct {
    v2 pos;
    v2 vel;
    v2 half_dim;
    Direction dir;
    b32 can_turn;
    f32 cornering_range;
} Actor;

typedef struct {
    Actor actor;
    Animation anim;
    PacManState state;
    PacManAnimType anim_type;
    Event caught;
    u32 anim_frame_counts[PACMAN_ANIM_TYPE_COUNT];
    u32 anim_indexes_in_sprite[PACMAN_ANIM_TYPE_COUNT];
} PacMan;

typedef struct {
    Actor actor;
    Animation anim;
    GhostType type;
    GhostState state;
    GhostAnimType anim_type;
    Event panic;
    Event start_recovery;
    Event recover;
    Event eaten;
    Event turned_to_eyes;
    u32 anim_frame_counts[GHOST_ANIM_TYPE_COUNT];
    u32 anim_indexes_in_sprite[GHOST_ANIM_TYPE_COUNT];
} Ghost;

typedef struct {
    GameState state;
    PacMan pacman;
    Ghost ghosts[GHOST_TYPE_COUNT];
    Event started;
    Event prelude;
    Event ready;
    Event round_started;
    Event pill_eaten;
    Event resume;
    Event round_over;
    u32 tick;
    u32 score;
    u32 dot_count;
    u32 pill_count;
    u32 xorshift;
} Game;

global Game game = {0};
global v2i dir_vectors[DIR_COUNT] = {{0, -1}, {-1, 0}, {0, 1}, {1, 0}};
global v2i ghost_scatter_targets[GHOST_TYPE_COUNT] = {
    {24, 4}, {3, 5}, {27, 33}, {3, 33}};
global v2 ghost_home_positions[GHOST_TYPE_COUNT] = {
    {120, 148}, {120, 148}, {104, 148}, {136, 148}};

internal f32 fabs(f32 x) { return x < 0 ? x * -1 : x; }

internal u32 xorshift32() {
    u32 x = game.xorshift;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return game.xorshift = x;
}

internal i32 dist_sq(v2i p1, v2i p2) {
    i32 x_diff = p1.x - p2.x;
    i32 y_diff = p1.y - p2.y;
    return ((x_diff * x_diff) + (y_diff * y_diff));
}

internal b32 in_range(v2 dist, f32 range) {
    return fabs(dist.x) < range && fabs(dist.y) < range ? 1 : 0;
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
    u32 tile_count = SCREEN_TILES_X * SCREEN_TILES_Y;
    u32 *result = (u32 *)MemAlloc(tile_count * sizeof(u32));
    u32 tile_map[SCREEN_TILES_Y][SCREEN_TILES_X] = {
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
         0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        // 0th row
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
         0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        // 1st row
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
         0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        // 2nd row
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
         0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        // 3rd row
        {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
         1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0},
        // 4th row
        {0, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1,
         1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 0},
        // 5th row
        {0, 1, 2, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 2, 1,
         1, 2, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 2, 1, 0},
        // 6th row
        {0, 1, 3, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 2, 1,
         1, 2, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 3, 1, 0},
        // 7th row
        {0, 1, 2, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 2, 1,
         1, 2, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 2, 1, 0},
        // 8th row
        {0, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
         2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 0},
        // 9th row
        {0, 1, 2, 1, 1, 1, 1, 2, 1, 1, 2, 1, 1, 1, 1,
         1, 1, 1, 1, 2, 1, 1, 2, 1, 1, 1, 1, 2, 1, 0},
        // 10th row
        {0, 1, 2, 1, 1, 1, 1, 2, 1, 1, 2, 1, 1, 1, 1,
         1, 1, 1, 1, 2, 1, 1, 2, 1, 1, 1, 1, 2, 1, 0},
        // 11th row
        {0, 1, 2, 2, 2, 2, 2, 2, 1, 1, 2, 2, 2, 2, 1,
         1, 2, 2, 2, 2, 1, 1, 2, 2, 2, 2, 2, 2, 1, 0},
        // 12th row
        {0, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 0, 1,
         1, 0, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 0},
        // 13th row
        {0, 0, 0, 0, 0, 0, 1, 2, 1, 1, 1, 1, 1, 0, 1,
         1, 0, 1, 1, 1, 1, 1, 2, 1, 0, 0, 0, 0, 0, 0},
        // 14th row
        {0, 0, 0, 0, 0, 0, 1, 2, 1, 1, 0, 0, 0, 0, 0,
         0, 0, 0, 0, 0, 1, 1, 2, 1, 0, 0, 0, 0, 0, 0},
        // 15th row
        {0, 0, 0, 0, 0, 0, 1, 2, 1, 1, 0, 1, 1, 1, 4,
         4, 1, 1, 1, 0, 1, 1, 2, 1, 0, 0, 0, 0, 0, 0},
        // 16th row
        {0, 1, 1, 1, 1, 1, 1, 2, 1, 1, 0, 1, 0, 0, 0,
         0, 0, 0, 1, 0, 1, 1, 2, 1, 1, 1, 1, 1, 1, 0},
        // 17th row
        {0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 1, 0, 0, 0,
         0, 0, 0, 1, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0},
        // 18th row
        {0, 1, 1, 1, 1, 1, 1, 2, 1, 1, 0, 1, 1, 1, 1,
         1, 1, 1, 1, 0, 1, 1, 2, 1, 1, 1, 1, 1, 1, 0},
        // 19th row
        {0, 0, 0, 0, 0, 0, 1, 2, 1, 1, 0, 1, 1, 1, 1,
         1, 1, 1, 1, 0, 1, 1, 2, 1, 0, 0, 0, 0, 0, 0},
        // 20th row
        {0, 0, 0, 0, 0, 0, 1, 2, 1, 1, 0, 0, 0, 0, 0,
         0, 0, 0, 0, 0, 1, 1, 2, 1, 0, 0, 0, 0, 0, 0},
        // 21st row
        {0, 0, 0, 0, 0, 0, 1, 2, 1, 1, 0, 1, 1, 1, 1,
         1, 1, 1, 1, 0, 1, 1, 2, 1, 0, 0, 0, 0, 0, 0},
        // 22nd row
        {0, 1, 1, 1, 1, 1, 1, 2, 1, 1, 0, 1, 1, 1, 1,
         1, 1, 1, 1, 0, 1, 1, 2, 1, 1, 1, 1, 1, 1, 0},
        // 23rd row
        {0, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1,
         1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 0},
        // 24th row
        {0, 1, 2, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 2, 1,
         1, 2, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 2, 1, 0},
        // 25th row
        {0, 1, 2, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 2, 1,
         1, 2, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 2, 1, 0},
        // 26th row
        {0, 1, 3, 2, 2, 1, 1, 2, 2, 2, 2, 2, 2, 2, 0,
         0, 2, 2, 2, 2, 2, 2, 2, 1, 1, 2, 2, 3, 1, 0},
        // 27th row
        {0, 1, 1, 1, 2, 1, 1, 2, 1, 1, 2, 1, 1, 1, 1,
         1, 1, 1, 1, 2, 1, 1, 2, 1, 1, 2, 1, 1, 1, 0},
        // 28th row
        {0, 1, 1, 1, 2, 1, 1, 2, 1, 1, 2, 1, 1, 1, 1,
         1, 1, 1, 1, 2, 1, 1, 2, 1, 1, 2, 1, 1, 1, 0},
        // 29th row
        {0, 1, 2, 2, 2, 2, 2, 2, 1, 1, 2, 2, 2, 2, 1,
         1, 2, 2, 2, 2, 1, 1, 2, 2, 2, 2, 2, 2, 1, 0},
        // 30th row
        {0, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1,
         1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 0},
        // 31st row
        {0, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1,
         1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 0},
        // 32nd row
        {0, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
         2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 0},
        // 33rd row
        {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
         1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0},
        // 34th row
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
         0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        // 35th row
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
         0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
         0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    };

    for (i32 y = 0; y < SCREEN_TILES_Y; y++) {
        for (i32 x = 0; x < SCREEN_TILES_X; x++) {
            result[(y * SCREEN_TILES_X) + x] = tile_map[y][x];
        }
    }
    return result;
}

internal Rectangle *get_sprite_tiles() {
    u32 tile_count = SPRITE_TILES_X * SPRITE_TILES_Y;
    Rectangle *result = (Rectangle *)MemAlloc(tile_count * sizeof(Rectangle));

    for (i32 y = 0; y < SPRITE_TILES_Y; y++) {
        for (i32 x = 0; x < SPRITE_TILES_X; x++) {
            result[(y * SPRITE_TILES_X) + x] = (Rectangle){
                (f32)SPRITE_TILE_WIDTH * x, (f32)SPRITE_TILE_HEIGHT * y,
                SPRITE_TILE_WIDTH, SPRITE_TILE_HEIGHT};
        }
    }

    return result;
}

internal void init_pacman(Rectangle *sprite_tiles) {
    PacMan *pacman = &game.pacman;
    pacman->actor = (Actor){0};
    pacman->actor.pos = (v2){120, 220};
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
    pacman->anim_indexes_in_sprite[PACMAN_DYING] = 16;

    pacman->anim_frame_counts[PACMAN_IDLING] = PACMAN_IDLE_ANIM_FRAME_COUNT;
    pacman->anim_frame_counts[PACMAN_GOING_LEFT] = PACMAN_MOVE_ANIM_FRAME_COUNT;
    pacman->anim_frame_counts[PACMAN_GOING_RIGHT] =
        PACMAN_MOVE_ANIM_FRAME_COUNT;
    pacman->anim_frame_counts[PACMAN_GOING_UP] = PACMAN_MOVE_ANIM_FRAME_COUNT;
    pacman->anim_frame_counts[PACMAN_GOING_DOWN] = PACMAN_MOVE_ANIM_FRAME_COUNT;
    pacman->anim_frame_counts[PACMAN_DYING] = PACMAN_DIE_ANIM_FRAME_COUNT;

    pacman->anim_type = PACMAN_GOING_LEFT;
    pacman->anim.frames =
        sprite_tiles + pacman->anim_indexes_in_sprite[pacman->anim_type];
    pacman->anim.frame_count = pacman->anim_frame_counts[pacman->anim_type];
    pacman->anim.frame_counter = 0;
    pacman->anim.ticks_per_anim_frame = PACMAN_TICKS_PER_ANIM_FRAME;
    pacman->anim.frame_index = 0;

    pacman->actor.cornering_range = PACMAN_CORNERING_RANGE;
    pacman->actor.can_turn = 1;
}

internal void init_blinky(Rectangle *sprite_tiles) {
    Ghost *ghost = &game.ghosts[GHOST_BLINKY];
    ghost->type = GHOST_BLINKY;
    ghost->state = GHOST_SCATTER;
    ghost->actor = (Actor){0};
    ghost->actor.pos = (v2){DOOR_ENTRY_X, DOOR_ENTRY_Y};
    ghost->actor.half_dim =
        (v2){SPRITE_TILE_WIDTH * 0.5, SPRITE_TILE_HEIGHT * 0.5};
    ghost->actor.vel = (v2){GHOST_SPEED, GHOST_SPEED};
    ghost->actor.dir = DIR_LEFT;

    ghost->anim_indexes_in_sprite[GHOST_GOING_LEFT] = 4 * SPRITE_TILES_X + 2;
    ghost->anim_indexes_in_sprite[GHOST_GOING_RIGHT] = 4 * SPRITE_TILES_X + 0;
    ghost->anim_indexes_in_sprite[GHOST_GOING_UP] = 4 * SPRITE_TILES_X + 4;
    ghost->anim_indexes_in_sprite[GHOST_GOING_DOWN] = 4 * SPRITE_TILES_X + 6;

    ghost->anim_indexes_in_sprite[GHOST_PANICKING] = 4 * SPRITE_TILES_X + 8;
    ghost->anim_indexes_in_sprite[GHOST_RECOVERING] = 4 * SPRITE_TILES_X + 8;

    ghost->anim_indexes_in_sprite[GHOST_EYES_LEFT] = 5 * SPRITE_TILES_X + 9;
    ghost->anim_indexes_in_sprite[GHOST_EYES_RIGHT] = 5 * SPRITE_TILES_X + 8;
    ghost->anim_indexes_in_sprite[GHOST_EYES_UP] = 5 * SPRITE_TILES_X + 10;
    ghost->anim_indexes_in_sprite[GHOST_EYES_DOWN] = 5 * SPRITE_TILES_X + 11;

    ghost->anim_frame_counts[GHOST_GOING_LEFT] = GHOST_MOVE_ANIM_FRAME_COUNT;
    ghost->anim_frame_counts[GHOST_GOING_RIGHT] = GHOST_MOVE_ANIM_FRAME_COUNT;
    ghost->anim_frame_counts[GHOST_GOING_UP] = GHOST_MOVE_ANIM_FRAME_COUNT;
    ghost->anim_frame_counts[GHOST_GOING_DOWN] = GHOST_MOVE_ANIM_FRAME_COUNT;

    ghost->anim_frame_counts[GHOST_PANICKING] = GHOST_PANIC_ANIM_FRAME_COUNT;
    ghost->anim_frame_counts[GHOST_RECOVERING] = GHOST_RECOVER_ANIM_FRAME_COUNT;

    ghost->anim_frame_counts[GHOST_EYES_LEFT] = GHOST_EYES_ANIM_FRAME_COUNT;
    ghost->anim_frame_counts[GHOST_EYES_RIGHT] = GHOST_EYES_ANIM_FRAME_COUNT;
    ghost->anim_frame_counts[GHOST_EYES_DOWN] = GHOST_EYES_ANIM_FRAME_COUNT;
    ghost->anim_frame_counts[GHOST_EYES_UP] = GHOST_EYES_ANIM_FRAME_COUNT;

    ghost->anim_type = GHOST_GOING_LEFT;
    ghost->anim.frames =
        sprite_tiles + ghost->anim_indexes_in_sprite[ghost->anim_type];
    ghost->anim.frame_count = ghost->anim_frame_counts[ghost->anim_type];
    ghost->anim.frame_counter = 0;
    ghost->anim.ticks_per_anim_frame = GHOST_TICKS_PER_ANIM_FRAME;
    ghost->anim.frame_index = 0;

    ghost->actor.cornering_range = GHOST_CORNERING_RANGE;
    ghost->actor.can_turn = 1;

    ghost->panic.tick = DISABLED_TICK;
    ghost->start_recovery.tick = DISABLED_TICK;
    ghost->recover.tick = DISABLED_TICK;
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

    if (next_tile.x < 0 || next_tile.x >= SCREEN_TILES_X) {
        return 1;
    }

    v2 dist_to_tile_mid = v2_sub(*next_pos, *curr_tile_pos);
    // TraceLog(LOG_DEBUG, "Tile center distance: [%d][%d]\n",
    // dist_to_tile_mid.x,
    //          dist_to_tile_mid.y);
    b32 result = 0;
    i32 tile_type = tile_map[(next_tile.y * SCREEN_TILES_X) + next_tile.x];
    b32 is_tile_occupied =
        tile_type == TILE_WALL || tile_type == TILE_DOOR ? 1 : 0;
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

internal void update_animation_frame(Animation *anim) {
    // TraceLog(LOG_DEBUG, "frame index: [%d]\n", anim->frame_index);
    anim->frame_counter++;
    if (anim->frame_counter >= anim->ticks_per_anim_frame) {
        anim->frame_index++;
        anim->frame_counter = 0;
    }
    if (anim->frame_index >= anim->frame_count) {
        anim->frame_index = 0;
    }
}

internal b32 is_now(u32 tick) { return tick == game.tick; }

internal u32 since(u32 tick) { return game.tick - tick; }

internal void after(Event *ev, u32 tick) { ev->tick = game.tick + tick; }

internal b32 is_red_zone(v2i tile) {
    return  tile.x > 11 && tile.x < 18 && (tile.y == 15 || tile.y == 27) ? 1 : 0;
}

internal b32 is_tunnes(v2i tile) {
    return 0;
}

internal void update_ghost(GhostType ghost_type, Rectangle *sprite_tiles,
                           u32 *tile_map, f32 dt) {
    Ghost *ghost = &game.ghosts[ghost_type];
    GhostState old_state = ghost->state;
    Direction old_dir = ghost->actor.dir;
    v2i curr_tile = get_tile(ghost->actor.pos);
    v2 curr_tile_pos = get_tile_pos(curr_tile);
    v2 dist_to_tile_mid = v2_sub(curr_tile_pos, ghost->actor.pos);
    v2 ghost_home_pos = ghost_home_positions[ghost_type];

    // ==================== GHOST STATE UPDATE ==================== //

    if (old_state == GHOST_EYES) {
        v2 dist_to_door =
            v2_sub(((v2){DOOR_ENTRY_X, DOOR_ENTRY_Y}), ghost->actor.pos);
        if (in_range(dist_to_door, GHOST_CORNERING_RANGE)) {
            TraceLog(LOG_DEBUG, "GHOST AT DOOR!! \n");
            ghost->state = GHOST_ENTER_HOME;
        }
    } else if (old_state == GHOST_ENTER_HOME) {
        v2 dist_to_home = v2_sub(ghost_home_pos, ghost->actor.pos);
        if (in_range(dist_to_home, GHOST_CORNERING_RANGE)) {
            ghost->state = GHOST_LEAVE_HOME;
        }
    } else if (old_state == GHOST_LEAVE_HOME) {
        v2 dist_to_door =
            v2_sub(((v2){DOOR_ENTRY_X, DOOR_ENTRY_Y}), ghost->actor.pos);
        if (in_range(dist_to_door, GHOST_CORNERING_RANGE)) {
            ghost->state = GHOST_SCATTER;
        }
    } else if (game.tick == ghost->panic.tick) {
        ghost->state = GHOST_PANIC;
    } else if (game.tick == ghost->start_recovery.tick) {
        ghost->state = GHOST_RECOVER;
    } else if (game.tick == ghost->eaten.tick) {
        ghost->state = GHOST_EATEN;
    } else if (game.tick == ghost->turned_to_eyes.tick) {
        ghost->state = GHOST_EYES;
        ghost->start_recovery.tick = DISABLED_TICK;
        ghost->recover.tick = DISABLED_TICK;
    } else if (game.tick == ghost->recover.tick || old_state == GHOST_CHASE ||
               old_state == GHOST_SCATTER) {
        u32 ticks_since_round_start = since(game.round_started.tick);
        if (ticks_since_round_start < 7 * FPS) {
            ghost->state = GHOST_SCATTER;
        } else if (ticks_since_round_start < 27 * FPS) {
            ghost->state = GHOST_CHASE;
        } else if (ticks_since_round_start < 34 * FPS) {
            ghost->state = GHOST_SCATTER;
        } else if (ticks_since_round_start < 54 * FPS) {
            ghost->state = GHOST_CHASE;
        } else if (ticks_since_round_start < 61 * FPS) {
            ghost->state = GHOST_SCATTER;
        } else {
            ghost->state = GHOST_CHASE;
        }
    }
    TraceLog(LOG_DEBUG, "GHOST STATE: %d\n", ghost->state);

    // ==================== GHOST DIRECTION UPDATE ==================== //

    b32 can_corner = 0;
    if (ghost->state == GHOST_HOME) {
        if (ghost->actor.pos.y >= 18 * TILE_HEIGHT) {
            ghost->actor.dir = DIR_UP;
        } else if (ghost->actor.pos.y <= 17 * TILE_HEIGHT) {
            ghost->actor.dir = DIR_DOWN;
        }
    } else if (ghost->state == GHOST_ENTER_HOME) {
        can_corner = fabs((f32)GHOST_HOME_CENTER_Y - ghost->actor.pos.y) <
                             GHOST_CORNERING_RANGE
                         ? 1
                         : 0;
        if (can_corner) {
            if (ghost->actor.pos.x > ghost_home_pos.x) {
                ghost->actor.dir = DIR_LEFT;
            } else {
                ghost->actor.dir = DIR_RIGHT;
            }
        } else {
            if (ghost->actor.dir != DIR_DOWN) {
                ghost->actor.dir = DIR_DOWN;
            }
        }
    } else if (ghost->state == GHOST_LEAVE_HOME) {
        can_corner = fabs((f32)GHOST_HOME_CENTER_X - ghost->actor.pos.x) <
                             GHOST_CORNERING_RANGE
                         ? 1
                         : 0;
        if (can_corner) {
            if (ghost->actor.dir != DIR_UP) {
                ghost->actor.dir = DIR_UP;
            }
        }
    } else if (!is_red_zone(curr_tile) ||
               is_red_zone(curr_tile) && (ghost->actor.dir == DIR_UP ||
                                          ghost->actor.dir == DIR_DOWN)) {
        TraceLog(LOG_DEBUG, "No Red Zone Tile: [%d][%d]\n", curr_tile.x, curr_tile.y);
        v2i dir_vec = dir_vectors[ghost->actor.dir];
        // TraceLog(LOG_DEBUG, "Direction BEFORE update: [%d]\n",
        // ghost->actor.dir);
        if (dir_vec.x != 0) {
            can_corner =
                fabs(dist_to_tile_mid.x) < ghost->actor.cornering_range;
        } else {
            can_corner =
                fabs(dist_to_tile_mid.y) < ghost->actor.cornering_range;
        }

        // Ensure that the ghost has moved out of the cornering region after a
        // turn else the ghost may backtrack at turns by making opposite turns
        // in 2 consecutive ticks.
        if (!ghost->actor.can_turn) {
            ghost->actor.can_turn = can_corner ? 0 : 1;
        }

        if (ghost->state != old_state && ghost->state != GHOST_RECOVER) {
            // Setting can_turn to 1 to allow backtracking on state change
            ghost->actor.can_turn = 1;
            ghost->actor.dir = get_opposite_dir(ghost->actor.dir);
        }

        if (can_corner && ghost->actor.can_turn) {
            Direction reverse_dir = get_opposite_dir(ghost->actor.dir);
            // TraceLog(LOG_DEBUG, "Current dir: %d\n", ghost->actor.dir);
            // TraceLog(LOG_DEBUG, "Reverse dir: %d\n", reverse_dir);
            v2i target_tile = (v2i){0, 0};

            switch (ghost->state) {
                case GHOST_SCATTER:
                    target_tile =
                        ghost_scatter_targets[ghost->type];
                    break;
                case GHOST_PANIC:
                case GHOST_RECOVER:
                    target_tile =
                        (v2i){xorshift32() % SCREEN_TILES_X,
                              xorshift32() % SCREEN_TILES_Y};
                    break;
                case GHOST_EYES:
                    target_tile = (v2i){14, 15};
                    break;
                default:
                    target_tile = get_tile(game.pacman.actor.pos);
                    break;
            }

            if (old_state == GHOST_LEAVE_HOME && ghost->state == GHOST_SCATTER) {
                ghost->actor.dir = DIR_LEFT;
            } else {
                Direction dirs[DIR_COUNT] = {DIR_UP, DIR_LEFT, DIR_DOWN, DIR_RIGHT};
                i32 closest_tile_dist_sq = 100000;
                for (i32 i = 0; i < DIR_COUNT; i++) {
                    Direction dir_to_check = dirs[i];

                    if (dir_to_check != reverse_dir) {
                        v2i dir_vec = dir_vectors[dir_to_check];
                        v2i tile_to_check = {curr_tile.x + dir_vec.x,
                                             curr_tile.y + dir_vec.y};
                        TraceLog(LOG_DEBUG, "Tile to check: [%d][%d]\n", tile_to_check.x, tile_to_check.y);
                        i32 tile_type =
                            tile_map[tile_to_check.y * SCREEN_TILES_X +
                                     tile_to_check.x];
                        TraceLog(LOG_DEBUG, "Tile type: %d\n", tile_type);

                        if ((tile_type != TILE_WALL &&
                             tile_type != TILE_DOOR) ||
                            (tile_type == TILE_DOOR &&
                             (ghost->state == GHOST_ENTER_HOME ||
                              ghost->state == GHOST_LEAVE_HOME))) {
                            i32 tile_to_check_dist_sq =
                                dist_sq(target_tile, tile_to_check);
                            TraceLog(LOG_DEBUG, "Tile distance: %d\n", tile_to_check_dist_sq);

                            if (tile_to_check_dist_sq < closest_tile_dist_sq) {
                                closest_tile_dist_sq = tile_to_check_dist_sq;
                                ghost->actor.dir = dir_to_check;
                            }
                        }
                    }
                }
            }
            if (old_dir != ghost->actor.dir) {
                ghost->actor.can_turn = 0;
            }
        }
    } else {
        TraceLog(LOG_DEBUG, "Red Zone Tile: [%d][%d]\n", curr_tile.x, curr_tile.y);
    }

    // ==================== GHOST VELOCITY UPDATE ==================== //

    if (ghost->state != old_state) {
        switch (ghost->state) {
            case GHOST_PANIC:
                ghost->actor.vel =
                    (v2){GHOST_FRIGHTENED_SPEED, GHOST_FRIGHTENED_SPEED};
                break;
            case GHOST_EYES:
                ghost->actor.vel = (v2){GHOST_EYES_SPEED, GHOST_EYES_SPEED};
                break;
            case GHOST_SCATTER:
            case GHOST_CHASE:
            case GHOST_LEAVE_HOME:
                ghost->actor.vel = (v2){GHOST_SPEED, GHOST_SPEED};
                break;
        }
    }

    // ==================== GHOST POSITION UPDATE ==================== //

    // Align ghost to tile-center.
    if (old_dir != ghost->actor.dir) {
        if (ghost->state == GHOST_ENTER_HOME && old_state != ghost->state) {
            ghost->actor.pos = (v2){DOOR_ENTRY_X, DOOR_ENTRY_Y};
        } else if (old_state == GHOST_LEAVE_HOME && old_state != ghost->state) {
            ghost->actor.pos = (v2){DOOR_ENTRY_X, DOOR_ENTRY_Y};
        } else if (ghost->state == GHOST_ENTER_HOME) {
            ghost->actor.pos.x = GHOST_HOME_CENTER_X;
        } else if (ghost->state == GHOST_LEAVE_HOME) {
            ghost->actor.pos.y = GHOST_HOME_CENTER_Y;
        } else {
            ghost->actor.pos = curr_tile_pos;
        }
    }
    v2i dir_vec = dir_vectors[ghost->actor.dir];
    TraceLog(LOG_DEBUG, "Direction AFTER update: [%d]\n", ghost->actor.dir);

    ghost->actor.pos =
        get_next_pos(&ghost->actor.pos, &ghost->actor.vel, &dir_vec, dt);
    if (ghost->actor.pos.x < TILE_WIDTH) {
        ghost->actor.pos.x = (f32)(BACK_BUFFER_WIDTH - TILE_WIDTH - 1);
    } else if (ghost->actor.pos.x >= (BACK_BUFFER_WIDTH - TILE_WIDTH)) {
        ghost->actor.pos.x = TILE_WIDTH;
    }

    // ==================== GHOST ANIMATION UPDATE ==================== //
    if (ghost->state != old_state) {
        if (ghost->state == GHOST_PANIC) {
            ghost->anim_type = GHOST_PANICKING;
        } else if (ghost->state == GHOST_RECOVER) {
            ghost->anim_type = GHOST_RECOVERING;
        } else if (ghost->state == GHOST_EYES) {
            switch (ghost->actor.dir) {
                case DIR_LEFT:
                    ghost->anim_type = GHOST_EYES_LEFT;
                    break;
                case DIR_RIGHT:
                    ghost->anim_type = GHOST_EYES_RIGHT;
                    break;
                case DIR_UP:
                    ghost->anim_type = GHOST_EYES_UP;
                    break;
                case DIR_DOWN:
                    ghost->anim_type = GHOST_EYES_DOWN;
                    break;
            }
        }
        ghost->anim.frames =
            sprite_tiles + ghost->anim_indexes_in_sprite[ghost->anim_type];
        ghost->anim.frame_count = ghost->anim_frame_counts[ghost->anim_type];
    }

    if (ghost->actor.dir != old_dir) {
        if (ghost->state == GHOST_EYES) {
            switch (ghost->actor.dir) {
                case DIR_LEFT:
                    ghost->anim_type = GHOST_EYES_LEFT;
                    break;
                case DIR_RIGHT:
                    ghost->anim_type = GHOST_EYES_RIGHT;
                    break;
                case DIR_UP:
                    ghost->anim_type = GHOST_EYES_UP;
                    break;
                case DIR_DOWN:
                    ghost->anim_type = GHOST_EYES_DOWN;
                    break;
            }
        } else if (ghost->state == GHOST_CHASE ||
                   ghost->state == GHOST_SCATTER ||
                   ghost->state == GHOST_HOME ||
                   ghost->state == GHOST_LEAVE_HOME) {
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
        }
        ghost->anim.frames =
            sprite_tiles + ghost->anim_indexes_in_sprite[ghost->anim_type];
        ghost->anim.frame_count = ghost->anim_frame_counts[ghost->anim_type];
    }
    update_animation_frame(&ghost->anim);
}

internal void update_pacman(Rectangle *sprite_tiles, u32 *tile_map, f32 dt) {
    PacMan *pacman = &game.pacman;
    if (pacman->state != PACMAN_CAUGHT && pacman->state != PACMAN_DEAD) {
        pacman->state = PACMAN_MOVING;
        v2i curr_tile = get_tile(pacman->actor.pos);
        v2 curr_tile_pos = {(curr_tile.x + 0.5f) * (f32)TILE_WIDTH,
                            (curr_tile.y + 0.5f) * (f32)TILE_HEIGHT};

        Direction next_dir = pacman->actor.dir;
        if (IsKeyDown(KEY_LEFT)) {
            next_dir = DIR_LEFT;
        }
        if (IsKeyDown(KEY_RIGHT)) {
            next_dir = DIR_RIGHT;
        }
        if (IsKeyDown(KEY_UP)) {
            next_dir = DIR_UP;
        }
        if (IsKeyDown(KEY_DOWN)) {
            next_dir = DIR_DOWN;
        }

        b32 is_dir_same = next_dir == pacman->actor.dir ? 1 : 0;
        v2i next_dir_vec = dir_vectors[next_dir];
        v2 next_pos = get_next_pos(&pacman->actor.pos, &pacman->actor.vel,
                                   &next_dir_vec, dt);
        v2 dist_to_tile_mid = v2_sub(next_pos, curr_tile_pos);

        b32 can_pacman_move = 0;
        // update check
        if (next_pos.x < TILE_WIDTH) {
            next_pos.x = (f32)(BACK_BUFFER_WIDTH - TILE_WIDTH - 1);
            can_pacman_move = 1;
        } else if (next_pos.x >= (BACK_BUFFER_WIDTH - TILE_WIDTH)) {
            next_pos.x = TILE_WIDTH;
            can_pacman_move = 1;
        } else {
            can_pacman_move =
                can_move(tile_map, &next_pos, &curr_tile, &curr_tile_pos,
                         &next_dir_vec, is_dir_same);

            if (can_pacman_move) {
                pacman->actor.dir = next_dir;
            } else {
                is_dir_same = 1;
                next_dir_vec = dir_vectors[pacman->actor.dir];
                next_pos = get_next_pos(&pacman->actor.pos, &pacman->actor.vel,
                                        &next_dir_vec, dt);
                can_pacman_move =
                    can_move(tile_map, &next_pos, &curr_tile, &curr_tile_pos,
                             &next_dir_vec, is_dir_same);
            }
        }

        // update
        if (can_pacman_move) {
            move(&pacman->actor.pos, &curr_tile_pos, &next_pos, &next_dir_vec,
                 is_dir_same);
            u32 curr_tile_type =
                tile_map[curr_tile.y * SCREEN_TILES_X + curr_tile.x];
            if (curr_tile_type == TILE_DOT ||
                curr_tile_type == TILE_PILL &&
                    in_range(dist_to_tile_mid, COLLISION_RANGE)) {
                if (curr_tile_type == TILE_PILL) {
                    game.pill_eaten.tick = game.tick;
                }
                tile_map[curr_tile.y * SCREEN_TILES_X + curr_tile.x] = 0;
            }
        } else if (is_dir_same) {
            resolve_wall_collision(&next_pos, &curr_tile_pos, &next_dir_vec);
            pacman->state = PACMAN_IDLE;
            pacman->anim_type = PACMAN_IDLING;
            pacman->anim.frame_counter = 0;
            pacman->anim.frames =
                sprite_tiles +
                pacman->anim_indexes_in_sprite[pacman->anim_type];
            pacman->anim.frame_count =
                pacman->anim_frame_counts[pacman->anim_type];
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

            pacman->anim.frames =
                sprite_tiles +
                pacman->anim_indexes_in_sprite[pacman->anim_type];
            pacman->anim.frame_count =
                pacman->anim_frame_counts[pacman->anim_type];
        }
    }

    update_animation_frame(&pacman->anim);
}

void update(Rectangle *sprite_tiles, u32 *tile_map, f32 dt) {
    PacMan *pacman = &game.pacman;
    update_pacman(sprite_tiles, tile_map, dt);
    if (pacman->state != PACMAN_DEAD) {
        update_ghost(GHOST_BLINKY, sprite_tiles, tile_map, dt);

        for (i32 i = 0; i < 1; i++) {
            Ghost *ghost = &game.ghosts[i];
            v2 dist_to_ghost = v2_sub(pacman->actor.pos, ghost->actor.pos);
            if (game.tick == game.pill_eaten.tick) {
                ghost->panic.tick = game.tick + 1;
                ghost->start_recovery.tick = ghost->panic.tick + 5 * FPS;
                ghost->recover.tick = ghost->panic.tick + 7 * FPS;
            } else if (in_range(dist_to_ghost, COLLISION_RANGE)) {
                if (ghost->state == GHOST_PANIC) {
                    ghost->eaten.tick = game.tick + 1;
                    game.state = GAME_PAUSE;
                    after(&ghost->turned_to_eyes, (1 * FPS));
                    TraceLog(LOG_DEBUG, "GHOST EATEN!!");
                    after(&game.resume, 1 * FPS);
                    after(&ghost->turned_to_eyes, (1 * FPS));
                } else if (ghost->state != GHOST_EYES) {
                    TraceLog(LOG_DEBUG, "Ghost state: %d\n", ghost->state);
                    game.pacman.caught.tick = game.tick;
                    pacman->state = PACMAN_CAUGHT;
                    TraceLog(LOG_DEBUG, "Pacman CAUGHT!!");
                    game.state = GAME_PAUSE;
                    after(&game.resume, 1 * FPS);
                    after(&game.round_over,
                          (1 * FPS) + ((PACMAN_DIE_ANIM_FRAME_COUNT - 1) * 8));
                }
            }
        }
    }
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

    Font font = LoadFontEx("assets/PressStart2P.ttf", 16, 0, 0);

    game.tick = 0;
    game.pill_eaten.tick = DISABLED_TICK;
    game.resume.tick = DISABLED_TICK;
    game.round_over.tick = DISABLED_TICK;
    game.ready.tick = game.tick;
    after(&game.round_started, 2 * FPS);
    game.state = GAME_READY;
    game.xorshift = 0x12345678;

    v2u maze_start_corner = {TILE_WIDTH, SCORE_TILE_ROW_COUNT * TILE_HEIGHT};
    u32 *tile_map = get_tile_map();
    Rectangle *sprite_tiles = get_sprite_tiles();

    init_pacman(sprite_tiles);
    PacMan *pacman = &game.pacman;
    init_blinky(sprite_tiles);
    Ghost *blinky = &game.ghosts[GHOST_BLINKY];

    Rectangle *dot_image = sprite_tiles + (3 * SPRITE_TILES_X);
    Animation pill_anim = {0};
    pill_anim.frames = sprite_tiles + (3 * SPRITE_TILES_X + 2);
    pill_anim.frame_count = 2;
    pill_anim.frame_counter = 0;
    pill_anim.ticks_per_anim_frame = 8;
    pill_anim.frame_index = 0;

    while (!WindowShouldClose()) {
        // TraceLog(LOG_DEBUG, "Pac-Man position BEFORE update:
        // [%3.2f][%3.2f]\n",
        //          pacman.pos.x, pacman.pos.y);
        // TraceLog(LOG_DEBUG, "Blinky position BEFORE update:
        // [%3.2f][%3.2f]\n",
        //          blinky->actor.pos.x, blinky->actor.pos.y);
        f32 dt = GetFrameTime();

        if (game.tick == game.round_started.tick ||
            game.tick == game.resume.tick) {
            TraceLog(LOG_DEBUG, "START / RESUME!");
            game.state = GAME_IN_PROGRESS;

            if (pacman->state == PACMAN_CAUGHT) {
                TraceLog(LOG_DEBUG, "Pacman DEAD!!");
                pacman->state = PACMAN_DEAD;
                pacman->anim_type = PACMAN_DYING;
                pacman->anim.frame_counter = 0;
                pacman->anim.frame_index = 0;
                pacman->anim.frames =
                    sprite_tiles +
                    pacman->anim_indexes_in_sprite[pacman->anim_type];
                pacman->anim.frame_count =
                    pacman->anim_frame_counts[pacman->anim_type];
                pacman->anim.ticks_per_anim_frame = 8;
            }
        }

        if (game.tick == game.round_over.tick) {
            game.state = GAME_ROUND_OVER;
        }

        if (game.state == GAME_IN_PROGRESS) {
            update(sprite_tiles, tile_map, dt);
            if (pacman->state != PACMAN_DEAD) {
                update_animation_frame(&pill_anim);
            }
        }

        BeginTextureMode(back_buffer);
        {
            ClearBackground(BLACK);

            DrawTexture(maze_tex, maze_start_corner.x, maze_start_corner.y,
                        WHITE);
            DrawTextEx(font, "HIGH SCORE", (v2){10 * TILE_WIDTH, TILE_HEIGHT},
                       8, 0, WHITE);
            if (game.state == GAME_READY) {
                DrawTextEx(font, "READY!",
                           (v2){12 * TILE_WIDTH, 21 * TILE_HEIGHT}, 8, 0,
                           YELLOW);
            }

            for (i32 y = 0; y < SCREEN_TILES_Y; y++) {
                for (i32 x = 0; x < SCREEN_TILES_X; x++) {
                    if (tile_map[y * SCREEN_TILES_X + x] == 2) {
                        DrawTextureRec(sprite_tex, *dot_image,
                                       (v2){(x - 0.5f) * (f32)TILE_WIDTH + 1,
                                            (y - 0.5f) * (f32)TILE_HEIGHT + 1},
                                       WHITE);
                    }
                    if (tile_map[y * SCREEN_TILES_X + x] == 3) {
                        DrawTextureRec(sprite_tex,
                                       pill_anim.frames[pill_anim.frame_index],
                                       (v2){(x - 0.5f) * (f32)TILE_WIDTH + 1,
                                            (y - 0.5f) * (f32)TILE_HEIGHT + 1},
                                       WHITE);
                    }
                }
            }

            if (game.state != GAME_ROUND_OVER) {
                DrawTextureRec(
                    sprite_tex, pacman->anim.frames[pacman->anim.frame_index],
                    (v2){pacman->actor.pos.x - pacman->actor.half_dim.x,
                         pacman->actor.pos.y - pacman->actor.half_dim.y},
                    WHITE);
            }

            if (pacman->state != PACMAN_DEAD && blinky->state != GHOST_EATEN) {
                DrawTextureRec(
                    sprite_tex, blinky->anim.frames[blinky->anim.frame_index],
                    (v2){blinky->actor.pos.x - blinky->actor.half_dim.x,
                         blinky->actor.pos.y - blinky->actor.half_dim.y},
                    WHITE);
            }
        }
        EndTextureMode();

        BeginDrawing();
        {
            DrawTexturePro(
                back_buffer.texture,
                (Rectangle){
                    TILE_WIDTH, TILE_HEIGHT,
                    (f32)(back_buffer.texture.width - 2 * TILE_WIDTH),
                    (f32)(-back_buffer.texture.height + 2 * TILE_HEIGHT)},
                (Rectangle){TILE_WIDTH * SCALE, TILE_HEIGHT * SCALE,
                            (f32)(screen_width - 2 * TILE_WIDTH * SCALE),
                            (f32)(screen_height - 2 * TILE_HEIGHT * SCALE)},
                (v2){0, 0}, 0.0f, WHITE);
        }
        EndDrawing();
        // TraceLog(LOG_DEBUG, "Pac-Man pos AFTER update: [%3.2f][%3.2f]\n\n\n",
        //          pacman.pos.x, pacman.pos.y);
        game.tick++;
    }
    CloseWindow();
    return 0;
}
