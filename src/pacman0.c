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
#define SPRITE_TILES_Y 13
#define SCALE 2.5f
#define DISABLED_TICK 0xFFFFFFFF
#define FPS 60
#define TIME_PER_FRAME 1 / FPS
#define PACMAN_TICKS_PER_ANIM_FRAME 4
#define PACMAN_TICKS_PER_DEATH_ANIM_FRAME 8
#define PACMAN_IDLE_ANIM_FRAME_COUNT 1
#define PACMAN_MOVE_ANIM_FRAME_COUNT 4
#define PACMAN_DIE_ANIM_FRAME_COUNT 11
#define PACMAN_CORNERING_RANGE 3.5f
#define GHOST_CORNERING_RANGE 2.5f
#define COLLISION_RANGE 3.5f
// #define MAX_SPEED 75.75757625f
#define MAX_SPEED 70.0f
#define GHOST_TICKS_PER_ANIM_FRAME 8
#define GHOST_MOVE_ANIM_FRAME_COUNT 2
#define GHOST_EATEN_ANIM_FRAME_COUNT 1
#define GHOST_EYES_ANIM_FRAME_COUNT 1
#define GHOST_PANIC_ANIM_FRAME_COUNT 2
#define GHOST_RECOVER_ANIM_FRAME_COUNT 4
#define PILL_TICKS_PER_ANIM_FRAME 8
#define MAZE_TICKS_PER_ANIM_FRAME 12
#define PRESS_ANY_KEY_TICKS_PER_ANIM_FRAME 30
#define DOT_COUNT 240
#define PILL_COUNT 4
#define DOOR_ENTRY_X (15 * TILE_WIDTH)
#define DOOR_ENTRY_Y ((15 + 0.5) * TILE_HEIGHT)
#define GHOST_HOME_CENTER_X (15 * TILE_WIDTH)
#define GHOST_HOME_CENTER_Y ((18 + 0.5) * TILE_HEIGHT)
#define ROUND_COUNT 3
#define BONUS_ACTIVE_TICKS 10 * FPS
#define BONUS_ACTIVE_TICKS 10 * FPS
#define BONUS_POINTS_TICKS 1 * FPS
#define FADE_TICKS 30

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
    BONUS_CHERRY,
    BONUS_STRAWBERRY,
    BONUS_PEACH,
    BONUS_APPLE,
    BONUS_GRAPES,
    BONUS_GALAXIAN,
    BONUS_BELL,
    BONUS_KEY,
    BONUS_TYPE_COUNT
} BonusType;

typedef enum { BONUS_ACTIVE, BONUS_POINTS, BONUS_INACTIVE } BonusState;

typedef enum {
    PACMAN_IDLE,
    PACMAN_MOVING,
    PACMAN_SPEEDING,
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
    GAME_LOAD,
    GAME_PRELUDE,
    GAME_READY,
    GAME_IN_PROGRESS,
    GAME_FROZEN,
    GAME_LEVEL_COMPLETE,
    GAME_ROUND_OVER,
    GAME_OVER,
    GAME_UNLOAD
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
    GHOST_EATEN_200,
    GHOST_EATEN_400,
    GHOST_EATEN_800,
    GHOST_EATEN_1600,
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
    BonusType type;
    BonusState state;
    Rectangle bonus_tile;
    Rectangle points_tile;
    u32 points;
} Bonus;

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
    v2i tile;
    u32 anim_frame_counts[PACMAN_ANIM_TYPE_COUNT];
    u32 anim_indexes_in_sprite[PACMAN_ANIM_TYPE_COUNT];
} PacMan;

typedef struct {
    Actor actor;
    Animation anim;
    GhostType type;
    GhostState state;
    GhostAnimType anim_type;
    Event eaten;
    Event turned_to_eyes;
    u32 anim_frame_counts[GHOST_ANIM_TYPE_COUNT];
    u32 anim_indexes_in_sprite[GHOST_ANIM_TYPE_COUNT];
} Ghost;

typedef struct {
    Bonus bonus;
    f32 pacman_speed;
    f32 pacman_dots_speed;
    f32 pacman_panic_speed;
    f32 pacman_panic_dots_speed;
    f32 ghost_speed;
    f32 ghost_home_speed;
    f32 ghost_eyes_speed;
    f32 ghost_tunnel_speed;
    f32 ghost_panic_speed;
    u32 elroy1_dots_left;
    f32 elroy1_speed;
    u32 elroy2_dots_left;
    f32 elroy2_speed;
    u32 ghost_panic_ticks;
    u32 ghost_flash_count;
    u32 inky_dot_limit;
    u32 clyde_dot_limit;
} Level;

typedef struct {
    GameState state;
    PacMan pacman;
    Ghost ghosts[GHOST_TYPE_COUNT];
    Sound chomp_sfx;
    Sound death_sfx;
    Sound bonus_sfx;
    Sound ghost_eat_sfx;
    Music siren_bgm;
    Music power_pellet_bgm;
    Level level;
    Event load;
    Event prelude;
    Event ready;
    Event play;
    Event pill_chomp;
    Event ghost_start_recovery;
    Event ghost_recover;
    Event freeze;
    Event resume;
    Event round_over;
    Event level_complete;
    Event unload;
    Event bonus_timeup;
    Event bonus_collected;
    Event bonus_point_hide;
    u32 tick;
    u32 score;
    u32 high_score;
    i32 rounds_left;
    u32 level_count;
    u32 dots_left;
    u32 pills_left;
    u32 ghost_eaten_count;
    u32 xorshift;
} Game;

global Game game = {0};
global v2i dir_vectors[DIR_COUNT] = {{0, -1}, {-1, 0}, {0, 1}, {1, 0}};
global v2i ghost_scatter_targets[GHOST_TYPE_COUNT] = {
    {24, 4}, {3, 5}, {27, 33}, {3, 33}};
global v2 bonus_pos = {120, 172};
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

internal v2i v2i_mul(v2i vec, i32 scalar) {
    return (v2i){vec.x * scalar, vec.y * scalar};
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

internal void init_tile_map(u32 *tile_map) {
    u32 tile_map_master[SCREEN_TILES_Y][SCREEN_TILES_X] = {
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
            tile_map[(y * SCREEN_TILES_X) + x] = tile_map_master[y][x];
        }
    }
}

internal u32 *get_tile_map() {
    u32 tile_count = SCREEN_TILES_X * SCREEN_TILES_Y;
    u32 *result = (u32 *)MemAlloc(tile_count * sizeof(u32));
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

internal Rectangle *get_maze_tiles() {
    u32 tile_count = 2;
    Rectangle *result = (Rectangle *)MemAlloc(tile_count * sizeof(Rectangle));

    Rectangle blue_maze = (Rectangle){0, 0, 28 * TILE_WIDTH, 31 * TILE_HEIGHT};
    Rectangle white_maze =
        (Rectangle){28 * TILE_WIDTH, 0, 28 * TILE_WIDTH, 31 * TILE_HEIGHT};

    result[0] = blue_maze;
    result[1] = white_maze;

    return result;
}

internal void init_pacman(Rectangle *sprite_tiles) {
    PacMan *pacman = &game.pacman;
    pacman->actor.can_turn = 1;
    pacman->actor.pos = (v2){120, 220};
    pacman->actor.vel = (v2){game.level.pacman_speed, game.level.pacman_speed};
    pacman->actor.dir = DIR_LEFT;
    pacman->state = PACMAN_MOVING;

    pacman->anim_type = PACMAN_GOING_LEFT;
    pacman->anim.frames =
        sprite_tiles + pacman->anim_indexes_in_sprite[pacman->anim_type];
    pacman->anim.frame_count = pacman->anim_frame_counts[pacman->anim_type];
    pacman->anim.frame_counter = 0;
    pacman->anim.ticks_per_anim_frame = PACMAN_TICKS_PER_ANIM_FRAME;
    pacman->anim.frame_index = 0;
}

internal void init_ghosts(Rectangle *sprite_tiles) {
    for (i32 i = 0; i < GHOST_TYPE_COUNT; i++) {
        Ghost *ghost = &game.ghosts[i];
        ghost->actor.can_turn = 1;

        switch (ghost->type) {
            case GHOST_BLINKY:
                ghost->state = GHOST_SCATTER;
                ghost->actor.pos = (v2){DOOR_ENTRY_X, DOOR_ENTRY_Y};
                ghost->actor.vel =
                    (v2){game.level.ghost_speed, game.level.ghost_speed};
                ghost->actor.dir = DIR_LEFT;
                ghost->anim_type = GHOST_GOING_LEFT;
                break;
            case GHOST_PINKY:
                ghost->state = GHOST_LEAVE_HOME;
                ghost->actor.pos =
                    (v2){GHOST_HOME_CENTER_X, GHOST_HOME_CENTER_Y};
                ghost->actor.vel = (v2){game.level.ghost_home_speed,
                                        game.level.ghost_home_speed};
                ghost->actor.dir = DIR_DOWN;
                ghost->anim_type = GHOST_GOING_DOWN;
                break;
            case GHOST_INKY:
                ghost->state = GHOST_HOME;
                ghost->actor.pos = (v2){GHOST_HOME_CENTER_X - (2 * TILE_WIDTH),
                                        GHOST_HOME_CENTER_Y};
                ghost->actor.vel = (v2){game.level.ghost_home_speed,
                                        game.level.ghost_home_speed};
                ghost->actor.dir = DIR_UP;
                ghost->anim_type = GHOST_GOING_UP;
                break;
            case GHOST_CLYDE:
                ghost->state = GHOST_HOME;
                ghost->actor.pos = (v2){GHOST_HOME_CENTER_X + (2 * TILE_WIDTH),
                                        GHOST_HOME_CENTER_Y};
                ghost->actor.vel = (v2){game.level.ghost_home_speed,
                                        game.level.ghost_home_speed};
                ghost->actor.dir = DIR_UP;
                ghost->anim_type = GHOST_GOING_UP;
                break;
        }

        ghost->anim.frames =
            sprite_tiles + ghost->anim_indexes_in_sprite[ghost->anim_type];
        ghost->anim.frame_count = ghost->anim_frame_counts[ghost->anim_type];
        ghost->anim.frame_counter = 0;
        ghost->anim.ticks_per_anim_frame = GHOST_TICKS_PER_ANIM_FRAME;
        ghost->anim.frame_index = 0;
    }
}

internal void init_round(Rectangle *sprite_tiles) {
    init_pacman(sprite_tiles);
    init_ghosts(sprite_tiles);
}

internal v2 get_next_pos(v2 *curr_pos, v2 *vel, v2i *dir_vec, f32 dt) {
    v2 pos_change = (v2){vel->x * dir_vec->x * dt, vel->y * dir_vec->y * dt};
    v2 next_pos = v2_add(*curr_pos, pos_change);
    return next_pos;
}

internal b32 can_move(u32 *tile_map, v2 *next_pos, v2i *curr_tile,
                      v2 *curr_tile_pos, v2i *dir_vec, b32 is_dir_same) {
    v2i next_tile = v2i_add(*curr_tile, *dir_vec);

    if (next_tile.x < 0 || next_tile.x >= SCREEN_TILES_X) {
        return 1;
    }

    v2 dist_to_tile_mid = v2_sub(*next_pos, *curr_tile_pos);
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

internal b32 in_red_zone(v2i tile) {
    return tile.x > 11 && tile.x < 18 && (tile.y == 15 || tile.y == 27) ? 1 : 0;
}

internal b32 in_tunnel(v2i tile) {
    return ((tile.x > 0 && tile.x < 7) || (tile.x > 22 && tile.x < 29)) &&
                   tile.y == 18
               ? 1
               : 0;
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

    if (old_state == GHOST_HOME) {
        u32 total_dots_eatens =
            (DOT_COUNT + PILL_COUNT) - (game.dots_left + game.pills_left);
        if (total_dots_eatens >= game.level.inky_dot_limit &&
            ghost->type == GHOST_INKY) {
            ghost->state = GHOST_LEAVE_HOME;
        }
        if (total_dots_eatens >= game.level.clyde_dot_limit &&
            ghost->type == GHOST_CLYDE) {
            ghost->state = GHOST_LEAVE_HOME;
        }
    } else if (old_state == GHOST_PANIC || old_state == GHOST_RECOVER) {
        v2 dist_to_pacman = v2_sub(game.pacman.actor.pos, ghost->actor.pos);
        if (in_range(dist_to_pacman, COLLISION_RANGE)) {
            ghost->eaten.tick = game.tick;
            game.freeze.tick = game.tick + 1;
            game.ghost_eaten_count += 1;
            PlaySound(game.ghost_eat_sfx);
            if (game.ghost_eaten_count == 1) {
                game.score += 200;
            } else if (game.ghost_eaten_count == 2) {
                game.score += 400;
            } else if (game.ghost_eaten_count == 3) {
                game.score += 800;
            } else if (game.ghost_eaten_count == 4) {
                game.score += 1600;
            }
            after(&ghost->turned_to_eyes, (1 * FPS));
            TraceLog(LOG_DEBUG, "GHOST EATEN!!");
            after(&game.resume, 1 * FPS);
            after(&ghost->turned_to_eyes, (1 * FPS));
        }
    } else if (old_state == GHOST_EYES) {
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
    }

    if (game.tick == game.pill_chomp.tick &&
        (ghost->state != GHOST_HOME && ghost->state != GHOST_ENTER_HOME &&
         ghost->state != GHOST_LEAVE_HOME)) {
        game.ghost_recover.tick = game.tick + game.level.ghost_panic_ticks;
        game.ghost_start_recovery.tick =
            game.ghost_recover.tick -
            (game.level.ghost_flash_count * GHOST_RECOVER_ANIM_FRAME_COUNT *
             GHOST_TICKS_PER_ANIM_FRAME);
        ghost->state = GHOST_PANIC;
    } else if (game.tick >= game.ghost_start_recovery.tick && ghost->state == GHOST_PANIC) {
        ghost->state = GHOST_RECOVER;
    } else if (game.tick == ghost->eaten.tick) {
        ghost->state = GHOST_EATEN;
    } else if (game.tick == ghost->turned_to_eyes.tick) {
        ghost->state = GHOST_EYES;
    } else if ((game.tick >= game.ghost_recover.tick && old_state == GHOST_RECOVER) || old_state == GHOST_CHASE ||
               old_state == GHOST_SCATTER) {
        u32 ticks_since_play = since(game.play.tick);
        if (ticks_since_play < 7 * FPS) {
            ghost->state = GHOST_SCATTER;
        } else if (ticks_since_play < 27 * FPS) {
            ghost->state = GHOST_CHASE;
        } else if (ticks_since_play < 34 * FPS) {
            ghost->state = GHOST_SCATTER;
        } else if (ticks_since_play < 54 * FPS) {
            ghost->state = GHOST_CHASE;
        } else if (ticks_since_play < 61 * FPS) {
            ghost->state = GHOST_SCATTER;
        } else {
            ghost->state = GHOST_CHASE;
        }
    }
    
    TraceLog(LOG_DEBUG, "GHOST STATE: %d\n", ghost->state);

    // ==================== GHOST DIRECTION UPDATE ==================== //

    b32 can_corner = 0;
    v2 dist_to_ghost_home_center = {0};
    if (ghost->state == GHOST_HOME) {
        if (ghost->actor.pos.y >= 19 * TILE_HEIGHT) {
            ghost->actor.dir = DIR_UP;
        } else if (ghost->actor.pos.y <= 18 * TILE_HEIGHT) {
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
            ghost->actor.dir = DIR_DOWN;
        }
    } else if (ghost->state == GHOST_LEAVE_HOME) {
        dist_to_ghost_home_center = v2_sub(
            (v2){GHOST_HOME_CENTER_X, GHOST_HOME_CENTER_Y}, ghost->actor.pos);
        if (fabs(dist_to_ghost_home_center.x) < GHOST_CORNERING_RANGE) {
             ghost->actor.dir = DIR_UP;
        } else if (fabs(dist_to_ghost_home_center.y) < GHOST_CORNERING_RANGE) {
            if (ghost->actor.pos.x > GHOST_HOME_CENTER_X) {
                ghost->actor.dir = DIR_LEFT;
            } else {
                ghost->actor.dir = DIR_RIGHT;
            }
        } else {
            if (ghost->actor.pos.y > GHOST_HOME_CENTER_Y) {
                ghost->actor.dir = DIR_UP;
            } else {
                ghost->actor.dir = DIR_DOWN;
            }
        }
    } else if (!in_red_zone(curr_tile) ||
               in_red_zone(curr_tile) && (ghost->actor.dir == DIR_UP ||
                                          ghost->actor.dir == DIR_DOWN)) {
        v2i dir_vec = dir_vectors[ghost->actor.dir];
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
            v2i target_tile = (v2i){0, 0};
            v2i pacman_tile = get_tile(game.pacman.actor.pos);

            switch (ghost->state) {
                case GHOST_SCATTER:
                    target_tile = ghost_scatter_targets[ghost->type];
                    break;
                case GHOST_PANIC:
                case GHOST_RECOVER:
                    target_tile = (v2i){xorshift32() % SCREEN_TILES_X,
                                        xorshift32() % SCREEN_TILES_Y};
                    break;
                case GHOST_EYES:
                    target_tile = (v2i){14, 15};
                    break;
                default:
                    switch (ghost->type) {
                        case GHOST_BLINKY:
                            target_tile = get_tile(game.pacman.actor.pos);
                            break;
                        case GHOST_PINKY:
                            target_tile = v2i_add(
                                pacman_tile,
                                v2i_mul(dir_vectors[game.pacman.actor.dir], 4));
                            break;
                        case GHOST_INKY:
                            v2i blinky_tile =
                                get_tile(game.ghosts[GHOST_BLINKY].actor.pos);
                            v2i two_ahead_pacman = v2i_add(
                                pacman_tile,
                                v2i_mul(dir_vectors[game.pacman.actor.dir], 2));
                            v2i dist = v2i_sub(two_ahead_pacman, blinky_tile);
                            target_tile =
                                v2i_add(blinky_tile, v2i_mul(dist, 2));
                            break;
                        case GHOST_CLYDE:
                            if (dist_sq(get_tile(ghost->actor.pos),
                                        pacman_tile) > 64) {
                                target_tile = pacman_tile;
                            } else {
                                target_tile =
                                    ghost_scatter_targets[GHOST_CLYDE];
                            }
                            break;
                    }
                    break;
            }

            if (old_state == GHOST_LEAVE_HOME &&
                ghost->state == GHOST_SCATTER) {
                ghost->actor.dir = DIR_LEFT;
            } else {
                Direction dirs[DIR_COUNT] = {DIR_UP, DIR_LEFT, DIR_DOWN,
                                             DIR_RIGHT};
                i32 closest_tile_dist_sq = 100000;
                for (i32 i = 0; i < DIR_COUNT; i++) {
                    Direction dir_to_check = dirs[i];

                    if (dir_to_check != reverse_dir) {
                        v2i dir_vec = dir_vectors[dir_to_check];
                        v2i tile_to_check = {curr_tile.x + dir_vec.x,
                                             curr_tile.y + dir_vec.y};
                        i32 tile_type =
                            tile_map[tile_to_check.y * SCREEN_TILES_X +
                                     tile_to_check.x];

                        if ((tile_type != TILE_WALL &&
                             tile_type != TILE_DOOR) ||
                            (tile_type == TILE_DOOR &&
                             (ghost->state == GHOST_ENTER_HOME ||
                              ghost->state == GHOST_LEAVE_HOME))) {
                            i32 tile_to_check_dist_sq =
                                dist_sq(target_tile, tile_to_check);

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
    }

    // ==================== GHOST VELOCITY UPDATE ==================== //

    switch (ghost->state) {
        case GHOST_CHASE:
        case GHOST_SCATTER:
            TraceLog(LOG_DEBUG, "CURR GHOST TILE: [%d][%d]\n", curr_tile.x,
                     curr_tile.y);
            if (in_tunnel(curr_tile)) {
                ghost->actor.vel = (v2){game.level.ghost_tunnel_speed,
                                        game.level.ghost_tunnel_speed};
            } else {
                ghost->actor.vel =
                    (v2){game.level.ghost_speed, game.level.ghost_speed};
            }

            if (ghost->type == GHOST_BLINKY) {
                u32 total_dot_left = game.dots_left + game.pills_left;
                if (total_dot_left <= game.level.elroy2_dots_left) {
                    ghost->actor.vel =
                        (v2){game.level.elroy2_speed, game.level.elroy2_speed};
                } else if (total_dot_left <= game.level.elroy1_dots_left) {
                    ghost->actor.vel =
                        (v2){game.level.elroy1_speed, game.level.elroy1_speed};
                }
            }
            break;
        case GHOST_PANIC:
            ghost->actor.vel = (v2){game.level.ghost_panic_speed,
                                    game.level.ghost_panic_speed};
            break;
        case GHOST_EYES:
        case GHOST_ENTER_HOME:
            ghost->actor.vel =
                (v2){game.level.ghost_eyes_speed, game.level.ghost_eyes_speed};
            break;
        case GHOST_LEAVE_HOME:
            ghost->actor.vel =
                (v2){game.level.ghost_home_speed, game.level.ghost_home_speed};
            break;
    }

    // ==================== GHOST POSITION UPDATE ==================== //

    // Align ghost to tile-center.
    if (old_dir != ghost->actor.dir && ghost->state != GHOST_HOME) {
        if (ghost->state == GHOST_ENTER_HOME || ghost->state == GHOST_LEAVE_HOME) {
            if (dist_to_ghost_home_center.x < GHOST_CORNERING_RANGE) {
                ghost->actor.pos.x = GHOST_HOME_CENTER_X;
            }
            if (dist_to_ghost_home_center.y < GHOST_CORNERING_RANGE) {
                ghost->actor.pos.y = GHOST_HOME_CENTER_Y;
            }
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

    TraceLog(LOG_DEBUG, "Ghost eaten count: [%d]\n", game.ghost_eaten_count);
    if (ghost->state != old_state) {
        TraceLog(LOG_DEBUG, "Ghost animation change as state changed.\n");
        switch (ghost->state) {
            case GHOST_PANIC:
                ghost->anim_type = GHOST_PANICKING;
                break;
            case GHOST_RECOVER:
                ghost->anim_type = GHOST_RECOVERING;
                break;
            case GHOST_EYES:
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
                break;
            case GHOST_EATEN:
                TraceLog(LOG_DEBUG, "Ghost eaten animation.\n");
                if (game.ghost_eaten_count == 1) {
                    ghost->anim_type = GHOST_EATEN_200;
                } else if (game.ghost_eaten_count == 2) {
                    ghost->anim_type = GHOST_EATEN_400;
                } else if (game.ghost_eaten_count == 3) {
                    ghost->anim_type = GHOST_EATEN_800;
                } else if (game.ghost_eaten_count == 4) {
                    ghost->anim_type = GHOST_EATEN_1600;
                }
                break;
            case GHOST_CHASE:
            case GHOST_SCATTER:
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
                break;
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
    PacManState old_state = pacman->state;
    Direction old_dir = pacman->actor.dir;

    if (pacman->state != PACMAN_CAUGHT && pacman->state != PACMAN_DEAD) {
        pacman->state = PACMAN_MOVING;
        v2i curr_tile = get_tile(pacman->actor.pos);
        v2 curr_tile_pos = {(curr_tile.x + 0.5f) * (f32)TILE_WIDTH,
                            (curr_tile.y + 0.5f) * (f32)TILE_HEIGHT};
        u32 curr_tile_type =
            tile_map[curr_tile.y * SCREEN_TILES_X + curr_tile.x];
        b32 has_dot_or_pill =
            curr_tile_type == TILE_DOT || curr_tile_type == TILE_PILL;

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

        if (game.tick == game.pill_chomp.tick) {
            pacman->state = PACMAN_SPEEDING;
            // Set pacman speed to normal when the ghosts recover
        } else if (game.tick == game.pill_chomp.tick + 1 + (7 * FPS)) {
            pacman->state = PACMAN_MOVING;
        }

        if (pacman->tile.x != curr_tile.x || pacman->tile.y != curr_tile.y) {
            pacman->tile = (v2i){curr_tile.x, curr_tile.y};

            if (has_dot_or_pill) {
                if (pacman->state == PACMAN_SPEEDING) {
                    pacman->actor.vel =
                        (v2){game.level.pacman_panic_dots_speed,
                             game.level.pacman_panic_dots_speed};
                } else {
                    pacman->actor.vel = (v2){game.level.pacman_dots_speed,
                                             game.level.pacman_dots_speed};
                }
            } else {
                if (pacman->state == PACMAN_SPEEDING) {
                    pacman->actor.vel = (v2){game.level.pacman_panic_speed,
                                             game.level.pacman_panic_speed};
                } else {
                    pacman->actor.vel =
                        (v2){game.level.pacman_speed, game.level.pacman_speed};
                }
            }
        }

        b32 is_dir_same = next_dir == pacman->actor.dir ? 1 : 0;
        v2i next_dir_vec = dir_vectors[next_dir];
        v2 next_pos = get_next_pos(&pacman->actor.pos, &pacman->actor.vel,
                                   &next_dir_vec, dt);

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
            v2 dist_to_tile_mid = v2_sub(next_pos, curr_tile_pos);
            v2 dist_to_bonus = v2_sub(next_pos, bonus_pos);
            move(&pacman->actor.pos, &curr_tile_pos, &next_pos, &next_dir_vec,
                 is_dir_same);

            if (game.level.bonus.state == BONUS_ACTIVE &&
                in_range(dist_to_bonus, COLLISION_RANGE)) {
                after(&game.bonus_collected, 1);
                after(&game.bonus_point_hide, 1 * FPS);
                game.score += game.level.bonus.points;
                PlaySound(game.bonus_sfx);
            }
            if (curr_tile_type == TILE_DOT ||
                curr_tile_type == TILE_PILL &&
                    in_range(dist_to_tile_mid, COLLISION_RANGE)) {
                if (curr_tile_type == TILE_DOT) {
                    PlaySound(game.chomp_sfx);
                    game.score += 10;
                    game.dots_left -= 1;
                }
                if (curr_tile_type == TILE_PILL) {
                    game.score += 50;
                    game.pill_chomp.tick = game.tick + 1;
                    game.pills_left -= 1;
                }

                tile_map[curr_tile.y * SCREEN_TILES_X + curr_tile.x] = 0;
            }
        } else if (is_dir_same) {
            resolve_wall_collision(&next_pos, &curr_tile_pos, &next_dir_vec);
            pacman->state = PACMAN_IDLE;
        }

        if (game.tick != game.pill_chomp.tick) {
            for (i32 i = 0; i < GHOST_TYPE_COUNT; i++) {
                Ghost *ghost = &game.ghosts[i];
                v2 dist_to_ghost = v2_sub(pacman->actor.pos, ghost->actor.pos);
                if (in_range(dist_to_ghost, COLLISION_RANGE)) {
                    if (ghost->state != GHOST_EYES &&
                        ghost->state != GHOST_EATEN &&
                        ghost->state != GHOST_PANIC &&
                        ghost->state != GHOST_RECOVER) {
                        TraceLog(LOG_DEBUG, "Ghost state: %d\n", ghost->state);
                        pacman->state = PACMAN_CAUGHT;
                        TraceLog(LOG_DEBUG, "Pacman CAUGHT!!");
                        game.state = GAME_FROZEN;
                        after(&game.resume, 1 * FPS);
                        after(&game.round_over,
                              (1 * FPS) + ((PACMAN_DIE_ANIM_FRAME_COUNT - 1) *
                                           PACMAN_TICKS_PER_DEATH_ANIM_FRAME));
                    }
                }
            }
        }
    } else if (pacman->state == PACMAN_CAUGHT &&
               game.tick == game.resume.tick) {
        PlaySound(game.death_sfx);
        TraceLog(LOG_DEBUG, "Pacman DEAD!!");
        pacman->state = PACMAN_DEAD;
    }

    if (pacman->state != PACMAN_CAUGHT) {
        if (old_state != pacman->state || old_dir != pacman->actor.dir) {
            // animate
            if (pacman->state == PACMAN_DEAD) {
                pacman->anim_type = PACMAN_DYING;
                pacman->anim.frame_counter = 0;
                pacman->anim.frame_index = 0;
                pacman->anim.ticks_per_anim_frame = PACMAN_TICKS_PER_DEATH_ANIM_FRAME;
            } else if (pacman->state == PACMAN_IDLE) {
                pacman->anim_type = PACMAN_IDLING;
            } else {
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
            }

            pacman->anim.frames =
                sprite_tiles +
                pacman->anim_indexes_in_sprite[pacman->anim_type];
            pacman->anim.frame_count =
                pacman->anim_frame_counts[pacman->anim_type];
        }
        update_animation_frame(&pacman->anim);
    }
}

void update(Rectangle *sprite_tiles, u32 *tile_map, f32 dt) {
    PacMan *pacman = &game.pacman;
    update_pacman(sprite_tiles, tile_map, dt);
    if (pacman->state != PACMAN_DEAD) {
        for (i32 i = 0; i < GHOST_TYPE_COUNT; i++) {
            update_ghost(i, sprite_tiles, tile_map, dt);
        }
    }
}

internal void init_level(u32 level_count, Rectangle *sprite_tiles, u32* tile_map) {
    Level *level = &game.level;

    if (level_count < 2) {
        level->bonus.type = BONUS_CHERRY;
        level->bonus.points = 100;
        level->bonus.bonus_tile = *(sprite_tiles + SPRITE_TILES_X + 13);
        level->bonus.points_tile = *(sprite_tiles + 9 * SPRITE_TILES_X);
    } else if (level_count < 3) {
        level->bonus.type = BONUS_STRAWBERRY;
        level->bonus.points = 300;
        level->bonus.bonus_tile = *(sprite_tiles + 2 * SPRITE_TILES_X);
        level->bonus.points_tile = *(sprite_tiles + 9 * SPRITE_TILES_X + 1);
    } else if (level_count < 5) {
        level->bonus.type = BONUS_PEACH;
        level->bonus.points = 500;
        level->bonus.bonus_tile = *(sprite_tiles + 2 * SPRITE_TILES_X + 1);
        level->bonus.points_tile = *(sprite_tiles + 9 * SPRITE_TILES_X + 2);
    } else if (level_count < 7) {
        level->bonus.type = BONUS_APPLE;
        level->bonus.points = 700;
        level->bonus.bonus_tile = *(sprite_tiles + 2 * SPRITE_TILES_X + 2);
        level->bonus.points_tile = *(sprite_tiles + 9 * SPRITE_TILES_X + 3);
    } else if (level_count < 9) {
        level->bonus.type = BONUS_GRAPES;
        level->bonus.points = 1000;
        level->bonus.bonus_tile = *(sprite_tiles + 2 * SPRITE_TILES_X + 4);
        level->bonus.points_tile =
            (Rectangle){4 * TILE_WIDTH, 9 * TILE_HEIGHT, 18, TILE_HEIGHT};
    } else if (level_count < 11) {
        level->bonus.type = BONUS_GALAXIAN;
        level->bonus.points = 2000;
        level->bonus.bonus_tile = *(sprite_tiles + 2 * SPRITE_TILES_X + 5);
        level->bonus.points_tile =
            (Rectangle){3 * TILE_WIDTH + 14, 10 * TILE_HEIGHT, 20, TILE_HEIGHT};
    } else if (level_count < 13) {
        level->bonus.type = BONUS_BELL;
        level->bonus.points = 3000;
        level->bonus.bonus_tile = *(sprite_tiles + 2 * SPRITE_TILES_X + 6);
        level->bonus.points_tile =
            (Rectangle){3 * TILE_WIDTH + 14, 11 * TILE_HEIGHT, 20, TILE_HEIGHT};
    } else {
        level->bonus.type = BONUS_KEY;
        level->bonus.points = 5000;
        level->bonus.bonus_tile = *(sprite_tiles + 2 * SPRITE_TILES_X + 7);
        level->bonus.points_tile =
            (Rectangle){3 * TILE_WIDTH + 14, 12 * TILE_HEIGHT, 20, TILE_HEIGHT};
    }
    level->bonus.state = BONUS_INACTIVE;

    if (level_count < 2) {
        level->pacman_speed = MAX_SPEED * 0.8f;
        level->pacman_dots_speed = MAX_SPEED * 0.71f;
        level->pacman_panic_speed = MAX_SPEED * 0.9f;
        level->pacman_panic_dots_speed = MAX_SPEED * 0.79f;
    } else if (level_count < 5) {
        level->pacman_speed = MAX_SPEED * 0.9f;
        level->pacman_dots_speed = MAX_SPEED * 0.79f;
        level->pacman_panic_speed = MAX_SPEED * 0.95f;
        level->pacman_panic_dots_speed = MAX_SPEED * 0.83f;
    } else if (level_count < 21) {
        level->pacman_speed = MAX_SPEED * 1.0f;
        level->pacman_dots_speed = MAX_SPEED * 0.87f;
        level->pacman_panic_speed = MAX_SPEED * 1.0f;
        level->pacman_panic_dots_speed = MAX_SPEED * 0.87f;
    } else {
        level->pacman_speed = MAX_SPEED * 0.9f;
        level->pacman_dots_speed = MAX_SPEED * 0.79f;
        level->pacman_panic_speed = MAX_SPEED * 1.0f;
        level->pacman_panic_dots_speed = MAX_SPEED * 0.87f;
    }

    if (level_count < 2) {
        level->ghost_speed = MAX_SPEED * 0.75f;
        level->ghost_tunnel_speed = MAX_SPEED * 0.4f;
        level->ghost_panic_speed = MAX_SPEED * 0.5f;
        level->elroy1_speed = MAX_SPEED * 0.8f;
        level->elroy2_speed = MAX_SPEED * 0.85f;
    } else if (level_count < 5) {
        level->ghost_speed = MAX_SPEED * 0.85f;
        level->ghost_tunnel_speed = MAX_SPEED * 0.45f;
        level->ghost_panic_speed = MAX_SPEED * 0.55f;
        level->elroy1_speed = MAX_SPEED * 0.90f;
        level->elroy2_speed = MAX_SPEED * 0.95f;
    } else {
        level->ghost_speed = MAX_SPEED * 0.95f;
        level->ghost_tunnel_speed = MAX_SPEED * 0.5f;
        level->ghost_panic_speed = MAX_SPEED * 0.6f;
        level->elroy1_speed = MAX_SPEED * 1.0f;
        level->elroy2_speed = MAX_SPEED * 1.05f;
    }
    level->ghost_home_speed = level->ghost_speed * 0.5f;
    level->ghost_eyes_speed = level->ghost_speed * 1.5f;

    if (level_count < 2) {
        level->elroy1_dots_left = 20;
        level->elroy2_dots_left = 10;
    } else if (level_count < 3) {
        level->elroy1_dots_left = 30;
        level->elroy2_dots_left = 15;
    } else if (level_count < 6) {
        level->elroy1_dots_left = 40;
        level->elroy2_dots_left = 20;
    } else if (level_count < 9) {
        level->elroy1_dots_left = 50;
        level->elroy2_dots_left = 25;
    } else if (level_count < 12) {
        level->elroy1_dots_left = 60;
        level->elroy2_dots_left = 30;
    } else if (level_count < 15) {
        level->elroy1_dots_left = 80;
        level->elroy2_dots_left = 40;
    } else if (level_count < 19) {
        level->elroy1_dots_left = 100;
        level->elroy2_dots_left = 50;
    } else {
        level->elroy1_dots_left = 120;
        level->elroy2_dots_left = 60;
    }

    if (level_count < 2) {
        level->ghost_panic_ticks = 6 * FPS;
    } else if (level_count < 3) {
        level->ghost_panic_ticks = 5 * FPS;
    } else if (level_count < 4) {
        level->ghost_panic_ticks = 4 * FPS;
    } else if (level_count < 5) {
        level->ghost_panic_ticks = 3 * FPS;
    } else if (level_count < 6) {
        level->ghost_panic_ticks = 2 * FPS;
    } else if (level_count < 7) {
        level->ghost_panic_ticks = 5 * FPS;
    } else if (level_count < 9) {
        level->ghost_panic_ticks = 2 * FPS;
    } else if (level_count < 10) {
        level->ghost_panic_ticks = 1 * FPS;
    } else if (level_count < 11) {
        level->ghost_panic_ticks = 5 * FPS;
    } else if (level_count < 12) {
        level->ghost_panic_ticks = 2 * FPS;
    } else if (level_count < 14) {
        level->ghost_panic_ticks = 1 * FPS;
    } else if (level_count < 15) {
        level->ghost_panic_ticks = 3 * FPS;
    } else if (level_count < 17) {
        level->ghost_panic_ticks = 1 * FPS;
    } else {
        level->ghost_panic_ticks = 1 * FPS;
    }

    if (level_count < 9) {
        level->ghost_flash_count = 5;
    } else if (level_count < 10) {
        level->ghost_flash_count = 3;
    } else if (level_count < 12) {
        level->ghost_flash_count = 5;
    } else if (level_count < 14) {
        level->ghost_flash_count = 3;
    } else if (level_count < 15) {
        level->ghost_flash_count = 5;
    } else {
        level->ghost_flash_count = 3;
    }

    if (level_count < 2) {
        level->inky_dot_limit = 30;
        level->clyde_dot_limit = 90;
    } else {
        level->inky_dot_limit = 0;
        level->clyde_dot_limit = 60;
    }

    game.dots_left = DOT_COUNT;
    game.pills_left = PILL_COUNT;
    if (game.state == GAME_LEVEL_COMPLETE) {
        init_tile_map(tile_map);
    }
}

internal void load_ghost(GhostType type) {
    Ghost *ghost = &game.ghosts[type];
    ghost->type = type;
    ghost->actor = (Actor){0};
    ghost->actor.half_dim =
        (v2){SPRITE_TILE_WIDTH * 0.5, SPRITE_TILE_HEIGHT * 0.5};
    ghost->actor.cornering_range = GHOST_CORNERING_RANGE;

    ghost->anim_indexes_in_sprite[GHOST_PANICKING] = 4 * SPRITE_TILES_X + 8;
    ghost->anim_indexes_in_sprite[GHOST_RECOVERING] = 4 * SPRITE_TILES_X + 8;

    ghost->anim_indexes_in_sprite[GHOST_EYES_LEFT] = 5 * SPRITE_TILES_X + 9;
    ghost->anim_indexes_in_sprite[GHOST_EYES_RIGHT] = 5 * SPRITE_TILES_X + 8;
    ghost->anim_indexes_in_sprite[GHOST_EYES_UP] = 5 * SPRITE_TILES_X + 10;
    ghost->anim_indexes_in_sprite[GHOST_EYES_DOWN] = 5 * SPRITE_TILES_X + 11;

    ghost->anim_indexes_in_sprite[GHOST_EATEN_200] = 8 * SPRITE_TILES_X;
    ghost->anim_indexes_in_sprite[GHOST_EATEN_400] = 8 * SPRITE_TILES_X + 1;
    ghost->anim_indexes_in_sprite[GHOST_EATEN_800] = 8 * SPRITE_TILES_X + 2;
    ghost->anim_indexes_in_sprite[GHOST_EATEN_1600] = 8 * SPRITE_TILES_X + 3;

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

    ghost->anim_frame_counts[GHOST_EATEN_200] = GHOST_EATEN_ANIM_FRAME_COUNT;
    ghost->anim_frame_counts[GHOST_EATEN_400] = GHOST_EATEN_ANIM_FRAME_COUNT;
    ghost->anim_frame_counts[GHOST_EATEN_800] = GHOST_EATEN_ANIM_FRAME_COUNT;
    ghost->anim_frame_counts[GHOST_EATEN_1600] = GHOST_EATEN_ANIM_FRAME_COUNT;

    switch (ghost->type) {
        case GHOST_BLINKY:
            ghost->anim_indexes_in_sprite[GHOST_GOING_LEFT] =
                4 * SPRITE_TILES_X + 2;
            ghost->anim_indexes_in_sprite[GHOST_GOING_RIGHT] =
                4 * SPRITE_TILES_X + 0;
            ghost->anim_indexes_in_sprite[GHOST_GOING_UP] =
                4 * SPRITE_TILES_X + 4;
            ghost->anim_indexes_in_sprite[GHOST_GOING_DOWN] =
                4 * SPRITE_TILES_X + 6;
            break;
        case GHOST_PINKY:
            ghost->anim_indexes_in_sprite[GHOST_GOING_LEFT] =
                5 * SPRITE_TILES_X + 2;
            ghost->anim_indexes_in_sprite[GHOST_GOING_RIGHT] =
                5 * SPRITE_TILES_X + 0;
            ghost->anim_indexes_in_sprite[GHOST_GOING_UP] =
                5 * SPRITE_TILES_X + 4;
            ghost->anim_indexes_in_sprite[GHOST_GOING_DOWN] =
                5 * SPRITE_TILES_X + 6;
            break;
        case GHOST_INKY:
            ghost->anim_indexes_in_sprite[GHOST_GOING_LEFT] =
                6 * SPRITE_TILES_X + 2;
            ghost->anim_indexes_in_sprite[GHOST_GOING_RIGHT] =
                6 * SPRITE_TILES_X + 0;
            ghost->anim_indexes_in_sprite[GHOST_GOING_UP] =
                6 * SPRITE_TILES_X + 4;
            ghost->anim_indexes_in_sprite[GHOST_GOING_DOWN] =
                6 * SPRITE_TILES_X + 6;
            break;
        case GHOST_CLYDE:
            ghost->anim_indexes_in_sprite[GHOST_GOING_LEFT] =
                7 * SPRITE_TILES_X + 2;
            ghost->anim_indexes_in_sprite[GHOST_GOING_RIGHT] =
                7 * SPRITE_TILES_X + 0;
            ghost->anim_indexes_in_sprite[GHOST_GOING_UP] =
                7 * SPRITE_TILES_X + 4;
            ghost->anim_indexes_in_sprite[GHOST_GOING_DOWN] =
                7 * SPRITE_TILES_X + 6;
            break;
    }
}

internal void load_pacman() {
    PacMan *pacman = &game.pacman;
    pacman->actor = (Actor){0};
    pacman->actor.half_dim =
        (v2){SPRITE_TILE_WIDTH * 0.5, SPRITE_TILE_HEIGHT * 0.5};
    pacman->actor.cornering_range = PACMAN_CORNERING_RANGE;

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
}

internal void init_game() {
    game.tick = 0;
    game.rounds_left = ROUND_COUNT;
    game.score = 0;
    game.level_count = 0;

    game.load.tick = DISABLED_TICK;
    game.prelude.tick = DISABLED_TICK;
    game.ready.tick = DISABLED_TICK;
    game.play.tick = DISABLED_TICK;
    game.pill_chomp.tick = DISABLED_TICK;
    game.ghost_start_recovery.tick = DISABLED_TICK;
    game.ghost_recover.tick = DISABLED_TICK;
    game.freeze.tick = DISABLED_TICK;
    game.resume.tick = DISABLED_TICK;
    game.round_over.tick = DISABLED_TICK;
    game.level_complete.tick = DISABLED_TICK;
    game.unload.tick = DISABLED_TICK;
    game.bonus_timeup.tick = DISABLED_TICK;
    game.bonus_collected.tick = DISABLED_TICK;
    game.bonus_point_hide.tick = DISABLED_TICK;
}

i32 main(void) {
    SetTraceLogCallback(trace_log_callback);
    SetTraceLogLevel(LOG_DEBUG);

    u32 screen_width = (u32)(BACK_BUFFER_WIDTH * SCALE);
    u32 screen_height = (u32)(BACK_BUFFER_HEIGHT * SCALE);

    InitWindow(screen_width, screen_height, "pacman0");
    InitAudioDevice();
    SetTargetFPS(FPS);

    Texture2D sprite_tex = LoadTexture("assets/sprite.png");
    Texture2D maze_tex = LoadTexture("assets/maze.png");
    RenderTexture2D back_buffer =
        LoadRenderTexture(BACK_BUFFER_WIDTH, BACK_BUFFER_HEIGHT);

    Font font = LoadFontEx("assets/PressStart2P.ttf", 16, 0, 0);

    Sound prelude = LoadSound("assets/prelude.wav");

    game.tick = 0;
    game.state = GAME_INTRO;
    game.xorshift = 0x12345678;
    init_game(&prelude);

    v2 maze_start_corner = {TILE_WIDTH, SCORE_TILE_ROW_COUNT * TILE_HEIGHT};
    u32 *tile_map = get_tile_map();
    Rectangle *sprite_tiles = get_sprite_tiles();
    Rectangle *maze_tiles = get_maze_tiles();

    load_pacman();
    PacMan *pacman = &game.pacman;
    game.chomp_sfx = LoadSound("assets/chomp.wav");
    game.death_sfx = LoadSound("assets/death.wav");
    game.bonus_sfx = LoadSound("assets/bonus.wav");
    game.ghost_eat_sfx = LoadSound("assets/ghost_eat.wav");
    game.siren_bgm = LoadMusicStream("assets/siren.wav");
    game.power_pellet_bgm = LoadMusicStream("assets/power_pellet.wav");

    for (i32 i = 0; i < GHOST_TYPE_COUNT; i++) {
        load_ghost(i);
    }

    Ghost *blinky = &game.ghosts[GHOST_BLINKY];
    Ghost *pinky = &game.ghosts[GHOST_PINKY];
    Ghost *inky = &game.ghosts[GHOST_INKY];
    Ghost *clyde = &game.ghosts[GHOST_CLYDE];

    Rectangle *dot_image = sprite_tiles + (3 * SPRITE_TILES_X);
    Animation pill_anim = {0};
    pill_anim.frames = sprite_tiles + (3 * SPRITE_TILES_X + 2);
    pill_anim.frame_count = 2;
    pill_anim.frame_counter = 0;
    pill_anim.ticks_per_anim_frame = PILL_TICKS_PER_ANIM_FRAME;
    pill_anim.frame_index = 0;
    Rectangle *life_indicator = sprite_tiles + (2 * SPRITE_TILES_X + 13);

    Animation maze_anim = {0};
    maze_anim.frames = maze_tiles;
    maze_anim.frame_count = 2;
    maze_anim.frame_counter = 0;
    maze_anim.ticks_per_anim_frame = MAZE_TICKS_PER_ANIM_FRAME;
    maze_anim.frame_index = 0;

    Animation press_any_key_anim = {0};
    press_any_key_anim.frame_count = 2;
    press_any_key_anim.frame_counter = 0;
    press_any_key_anim.ticks_per_anim_frame = PRESS_ANY_KEY_TICKS_PER_ANIM_FRAME;
    press_any_key_anim.frame_index = 0;

    f32 alpha = 0.0f;
    while (!WindowShouldClose()) {
#if DEBUG
        f32 dt = TIME_PER_FRAME;
#else
        f32 dt = GetFrameTime();
#endif
        if (game.state == GAME_INTRO) {
            if (GetKeyPressed() != 0) {
                game.state = GAME_LOAD;
                game.load.tick = game.tick + 30;
                init_tile_map(tile_map);
            }

            if (game.tick <= 30) {
                alpha += 0.033333f;
            } else {
                alpha = 1.0f;
            }
        } else if (game.state == GAME_LOAD) {
            if (game.tick < game.load.tick) {
                alpha -= 0.033333f;
            } else if (game.tick == game.load.tick) {
                alpha = 0.0f;
                game.tick = 0;
                game.state = GAME_PRELUDE;
                after(&game.ready, 2 * FPS);
                PlaySound(prelude);
            }
        } else if (game.state == GAME_UNLOAD) {
            alpha -= 0.033333f;
            if (game.tick >= (game.unload.tick + 30)) {
                game.state = GAME_INTRO;
                init_game();
            }
        } else if (game.state == GAME_PRELUDE && game.tick <= 30) {
            if (game.tick < 30) {
                alpha += 0.033333f;
            } else if (game.tick == 30) {
                alpha = 1.0f;
            }
        } else {
            if (game.tick == game.unload.tick) {
                game.state = GAME_UNLOAD;
            } else if (game.tick == game.freeze.tick) {
                game.state = GAME_FROZEN;
            } else if (game.tick == game.pill_chomp.tick) {
                game.ghost_eaten_count = 0;
                StopMusicStream(game.siren_bgm);
                PlayMusicStream(game.power_pellet_bgm);
            } else if (game.tick == game.play.tick ||
                    game.tick == game.resume.tick) {
                TraceLog(LOG_DEBUG, "START / RESUME!");
                game.state = GAME_IN_PROGRESS;
                PlayMusicStream(game.siren_bgm);
            } else if (game.tick == game.ready.tick) {
                if (game.state == GAME_LEVEL_COMPLETE ||
                        game.state == GAME_PRELUDE) {
                    game.level_count += 1;
                    init_level(game.level_count, sprite_tiles, tile_map);
                    init_round(sprite_tiles);
                }
                if (game.state == GAME_ROUND_OVER ||
                    game.state == GAME_PRELUDE) {
                    game.rounds_left -= 1;
                }
                if (game.state == GAME_ROUND_OVER) {
                    init_round(sprite_tiles);
                }
                game.state = GAME_READY;
                after(&game.play, 3 * FPS);
            } else if (game.tick == game.round_over.tick) {
                game.state = GAME_ROUND_OVER;
                after(&game.ready, 2 * FPS);
            } else if (game.tick == game.level_complete.tick) {
                game.state = GAME_LEVEL_COMPLETE;
                after(&game.ready, 16 * MAZE_TICKS_PER_ANIM_FRAME);
            } else if (game.pills_left == 0 && game.dots_left == 0 &&
                    game.state == GAME_IN_PROGRESS) {
                game.state = GAME_FROZEN;
                after(&game.level_complete, 1 * FPS);
            }

            u32 total_dots_eatens =
                (DOT_COUNT + PILL_COUNT) - (game.dots_left + game.pills_left);

            if (total_dots_eatens == 70 || total_dots_eatens == 170) {
                game.level.bonus.state = BONUS_ACTIVE;
                after(&game.bonus_timeup, 10 * FPS);
            }

            if (game.tick == game.bonus_collected.tick) {
                game.bonus_timeup.tick = DISABLED_TICK;
                game.level.bonus.state = BONUS_POINTS;
            }

            if (game.tick == game.bonus_timeup.tick ||
                game.tick == game.bonus_point_hide.tick) {
                game.level.bonus.state = BONUS_INACTIVE;
            }

            if (game.rounds_left < 0 && game.state != GAME_OVER &&
                game.state != GAME_UNLOAD) {
                game.state = GAME_OVER;
                after(&game.unload, 2 * FPS);
            }

            if (game.state == GAME_IN_PROGRESS) {
                update(sprite_tiles, tile_map, dt);
                if (pacman->state != PACMAN_DEAD) {
                    update_animation_frame(&pill_anim);
                }
                if (game.tick > game.ghost_recover.tick) {
                    if (IsMusicStreamPlaying(game.power_pellet_bgm)) {
                        StopMusicStream(game.power_pellet_bgm);
                        PlayMusicStream(game.siren_bgm);
                    }
                }
                if (game.pill_chomp.tick <= game.tick && game.tick <= game.ghost_recover.tick) {
                    UpdateMusicStream(game.power_pellet_bgm);
                } else {
                    UpdateMusicStream(game.siren_bgm);
                }
            } else if (game.state == GAME_LEVEL_COMPLETE) {
                update_animation_frame(&maze_anim);
            }

            if (game.score > game.high_score) {
                game.high_score = game.score;
            }
        }

        BeginTextureMode(back_buffer);
        {
            ClearBackground(BLACK);

            DrawTextEx(font, "HIGH SCORE", (v2){10 * TILE_WIDTH, TILE_HEIGHT},
                       8, 0, Fade(WHITE, alpha));

            char score_text[10];
            if (game.score < 10) {
                sprintf_s(score_text, sizeof(score_text), "0%d", game.score);
            } else {
                sprintf_s(score_text, sizeof(score_text), "%d", game.score);
            }
            DrawTextEx(font, score_text, (v2){6 * TILE_WIDTH, 2 * TILE_HEIGHT},
                       8, 0, Fade(WHITE, alpha));

            if (game.high_score) {
                char high_score_text[10];
                if (game.high_score < 10) {
                    sprintf_s(high_score_text, sizeof(high_score_text), "0%d",
                              game.high_score);
                } else {
                    sprintf_s(high_score_text, sizeof(high_score_text), "%d",
                              game.high_score);
                }
                DrawTextEx(font, high_score_text,
                           (v2){15 * TILE_WIDTH, 2 * TILE_HEIGHT}, 8, 0, Fade(WHITE, alpha));
            }

            // ====================== DRAW INTRO SCREEN ======================

            if (game.state == GAME_INTRO || game.state == GAME_LOAD) {
                DrawTextEx(font, "1UP", (v2){4 * TILE_WIDTH, TILE_HEIGHT},
                        8, 0, Fade(WHITE, alpha));

                DrawTextEx(font, "2UP", (v2){23 * TILE_WIDTH, TILE_HEIGHT},
                        8, 0, Fade(WHITE, alpha));

                DrawTextEx(font, "CHARACTER / NICKNAME", (v2){8 * TILE_WIDTH, 6 * TILE_HEIGHT},
                        8, 0, Fade(WHITE, alpha));
                // BLINKY
                if (game.tick > 60) {
                    DrawTextureRec(sprite_tex,
                                   *(sprite_tiles + (4 * SPRITE_TILES_X)),
                                   (v2){5 * TILE_WIDTH, 8 * TILE_HEIGHT}, Fade(WHITE, alpha));
                }
                if (game.tick > 120) {
                    DrawTextEx(font, "-SHADOW",
                            (v2){8 * TILE_WIDTH, 8.5 * TILE_HEIGHT}, 8, 0,
                            Fade((Color){255, 0, 0, 255}, alpha));
                }
                if (game.tick > 150) {
                    DrawTextEx(font, "BLINKY",
                            (v2){18 * TILE_WIDTH, 8.5 * TILE_HEIGHT}, 8, 0,
                            Fade((Color){255, 0, 0, 255}, alpha));
                }

                // PINKY
                if (game.tick > 210) {
                    DrawTextureRec(sprite_tex,
                                   *(sprite_tiles + (5 * SPRITE_TILES_X)),
                                   (v2){5 * TILE_WIDTH, 11 * TILE_HEIGHT}, Fade(WHITE, alpha));
                }
                if (game.tick > 270) {
                    DrawTextEx(font, "-SPEEDY",
                            (v2){8 * TILE_WIDTH, 11.5 * TILE_HEIGHT}, 8, 0,
                            Fade((Color){252, 181, 255, 255}, alpha));
                }
                if (game.tick > 300) {
                    DrawTextEx(font, "PINKY",
                            (v2){18 * TILE_WIDTH, 11.5 * TILE_HEIGHT}, 8, 0,
                            Fade((Color){252, 181, 255, 255}, alpha));
                }

                // INKY
                if (game.tick > 360) {
                    DrawTextureRec(sprite_tex,
                                   *(sprite_tiles + (6 * SPRITE_TILES_X)),
                                   (v2){5 * TILE_WIDTH, 14 * TILE_HEIGHT}, Fade(WHITE, alpha));
                }
                if (game.tick > 420) {
                    DrawTextEx(font, "-BASHFUL",
                            (v2){8 * TILE_WIDTH, 14.5 * TILE_HEIGHT}, 8, 0,
                            Fade((Color){0, 255, 255, 255}, alpha));
                }
                if (game.tick > 450) {
                    DrawTextEx(font, "INKY",
                            (v2){18 * TILE_WIDTH, 14.5 * TILE_HEIGHT}, 8, 0,
                            Fade((Color){0, 255, 255, 255}, alpha));
                }

                // CLYDE
                if (game.tick > 510) {
                    DrawTextureRec(sprite_tex,
                                   *(sprite_tiles + (7 * SPRITE_TILES_X)),
                                   (v2){5 * TILE_WIDTH, 17 * TILE_HEIGHT}, Fade(WHITE, alpha));
                }
                if (game.tick > 570) {
                    DrawTextEx(font, "-POKEY",
                            (v2){8 * TILE_WIDTH, 17.5 * TILE_HEIGHT}, 8, 0,
                            Fade((Color){248, 187, 85, 255}, alpha));
                }
                if (game.tick > 600) {
                    DrawTextEx(font, "CLYDE",
                            (v2){18 * TILE_WIDTH, 17.5 * TILE_HEIGHT}, 8, 0,
                            Fade((Color){248, 187, 85, 255}, alpha));
                }

                if (game.tick > 660) {
                    DrawTextureRec(sprite_tex,
                                   *(sprite_tiles + (3 * SPRITE_TILES_X + 1)),
                                   (v2){11 * TILE_WIDTH, 25 * TILE_HEIGHT}, Fade(WHITE, alpha));
                    DrawTextEx(font, "10",
                            (v2){13.5 * TILE_WIDTH, 25.5 * TILE_HEIGHT}, 8, 0,
                            WHITE);
                    DrawTextEx(font, "PTS",
                            (v2){16.5 * TILE_WIDTH, 25.5 * TILE_HEIGHT + 2}, 6, 0,
                            WHITE);
                    DrawTextureRec(sprite_tex,
                                   *(sprite_tiles + (3 * SPRITE_TILES_X + 2)),
                                   (v2){11 * TILE_WIDTH, 27 * TILE_HEIGHT}, Fade(WHITE, alpha));
                    DrawTextEx(font, "50",
                            (v2){13.5 * TILE_WIDTH, 27.5 * TILE_HEIGHT}, 8, 0,
                            WHITE);
                    DrawTextEx(font, "PTS",
                            (v2){16.5 * TILE_WIDTH, 27.5 * TILE_HEIGHT + 2}, 6, 0,
                            WHITE);
                }

                if (game.tick > 720) {
                    update_animation_frame(&press_any_key_anim);
                    if (press_any_key_anim.frame_index == 0) {
                        DrawTextEx(font, "PRESS ANY KEY TO START!",
                                   (v2){4 * TILE_WIDTH, 32 * TILE_HEIGHT}, 8, 0,
                                   Fade((Color){252, 181, 255, 255}, alpha));
                    }
                }

                DrawTextEx(font, "CREDIT  0", (v2){4 * TILE_WIDTH, 36 * TILE_HEIGHT},
                        8, 0, Fade(WHITE, alpha));
            } else {
            // ====================== DRAW MAIN SCREEN =======================
                if (game.state == GAME_LEVEL_COMPLETE) {
                    DrawTextureRec(maze_tex,
                                maze_anim.frames[maze_anim.frame_index],
                                maze_start_corner, Fade(WHITE, alpha));
                } else {
                    DrawTextureRec(maze_tex, maze_anim.frames[0], maze_start_corner,
                                Fade(WHITE, alpha));
                }

                for (i32 i = 0; i < game.rounds_left; i++) {
                    DrawTextureRec(
                        sprite_tex, *life_indicator,
                        (v2){(f32)(i * 2 + 3) * TILE_WIDTH, 35 * TILE_HEIGHT},
                        Fade(WHITE, alpha));
                }

                for (i32 i = 0; i < (game.level.bonus.type + 1); i++) {
                    DrawTextureRec(
                        sprite_tex, *(sprite_tiles + SPRITE_TILES_X + 13 + i),
                        (v2){(f32)(25 - i * 2) * TILE_WIDTH, 35 * TILE_HEIGHT},
                        Fade(WHITE, alpha));
                }

                if (game.state == GAME_PRELUDE) {
                    DrawTextEx(font, "PLAYER ONE",
                            (v2){10 * TILE_WIDTH, 15 * TILE_HEIGHT}, 8, 0,
                            Fade((Color){0, 255, 255, 255}, alpha));
                }
                if (game.state == GAME_PRELUDE || game.state == GAME_READY) {
                    DrawTextEx(font, "READY!",
                            (v2){12 * TILE_WIDTH, 21 * TILE_HEIGHT}, 8, 0,
                            Fade((Color){255, 255, 0, 255}, alpha));
                }
                if (game.state == GAME_OVER) {
                    DrawTextEx(font, "GAME  OVER",
                               (v2){10 * TILE_WIDTH, 21 * TILE_HEIGHT}, 8, 0,
                               Fade((Color){255, 0, 0, 255}, alpha));
                }

                if (game.level.bonus.state == BONUS_ACTIVE) {
                    DrawTextureRec(sprite_tex, game.level.bonus.bonus_tile,
                                (v2){bonus_pos.x - 0.5f * SPRITE_TILE_WIDTH,
                                        bonus_pos.y - 0.5f * SPRITE_TILE_HEIGHT},
                                Fade(WHITE, alpha));
                } else if (game.level.bonus.state == BONUS_POINTS) {
                    DrawTextureRec(
                        sprite_tex, game.level.bonus.points_tile,
                        (v2){
                            bonus_pos.x - 0.5f * game.level.bonus.points_tile.width,
                            bonus_pos.y -
                                0.5f * game.level.bonus.points_tile.height},
                        Fade(WHITE, alpha));
                }

                for (i32 y = 0; y < SCREEN_TILES_Y; y++) {
                    for (i32 x = 0; x < SCREEN_TILES_X; x++) {
                        if (tile_map[y * SCREEN_TILES_X + x] == 2) {
                            DrawTextureRec(sprite_tex, *dot_image,
                                        (v2){(x - 0.5f) * (f32)TILE_WIDTH + 1,
                                                (y - 0.5f) * (f32)TILE_HEIGHT + 1},
                                        Fade(WHITE, alpha));
                        }
                        if (tile_map[y * SCREEN_TILES_X + x] == 3) {
                            DrawTextureRec(sprite_tex,
                                        pill_anim.frames[pill_anim.frame_index],
                                        (v2){(x - 0.5f) * (f32)TILE_WIDTH + 1,
                                                (y - 0.5f) * (f32)TILE_HEIGHT + 1},
                                        Fade(WHITE, alpha));
                        }
                    }
                }

                if (game.state != GAME_PRELUDE &&
                    game.state != GAME_ROUND_OVER && game.state != GAME_OVER &&
                    game.state != GAME_LEVEL_COMPLETE && game.state != GAME_UNLOAD) {
                    DrawTextureRec(
                        sprite_tex, pacman->anim.frames[pacman->anim.frame_index],
                        (v2){pacman->actor.pos.x - pacman->actor.half_dim.x,
                            pacman->actor.pos.y - pacman->actor.half_dim.y},
                        Fade(WHITE, alpha));
                    if (game.pacman.state != PACMAN_DEAD) {
                        DrawTextureRec(
                            sprite_tex,
                            blinky->anim.frames[blinky->anim.frame_index],
                            (v2){blinky->actor.pos.x - blinky->actor.half_dim.x,
                                blinky->actor.pos.y - blinky->actor.half_dim.y},
                            Fade(WHITE, alpha));
                        DrawTextureRec(
                            sprite_tex,
                            pinky->anim.frames[pinky->anim.frame_index],
                            (v2){pinky->actor.pos.x - pinky->actor.half_dim.x,
                                pinky->actor.pos.y - pinky->actor.half_dim.y},
                            Fade(WHITE, alpha));
                        DrawTextureRec(
                            sprite_tex,
                            inky->anim.frames[inky->anim.frame_index],
                            (v2){inky->actor.pos.x - inky->actor.half_dim.x,
                                inky->actor.pos.y - inky->actor.half_dim.y},
                            Fade(WHITE, alpha));
                        DrawTextureRec(
                            sprite_tex,
                            clyde->anim.frames[clyde->anim.frame_index],
                            (v2){clyde->actor.pos.x - clyde->actor.half_dim.x,
                                clyde->actor.pos.y - clyde->actor.half_dim.y},
                            Fade(WHITE, alpha));
                    }
                }
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
        game.tick++;
    }

    UnloadSound(prelude);
    UnloadSound(game.chomp_sfx);
    UnloadSound(game.death_sfx);
    UnloadSound(game.bonus_sfx);
    UnloadSound(game.ghost_eat_sfx);
    UnloadMusicStream(game.siren_bgm);
    CloseAudioDevice();
    CloseWindow();

    return 0;
}
