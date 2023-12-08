#ifndef DEFINES_H

#include <stdint.h>
#include "raylib.h"

#define global static
#define local static
#define internal static

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

typedef signed char i8;
typedef signed short i16;
typedef signed int i32;
typedef signed long long i64;

typedef float f32;
typedef double f64;

typedef int b32;
typedef char b8;

typedef Vector2 v2;

typedef struct v2u {
    u32 x;
    u32 y;
} v2u;

typedef struct v2i {
    i32 x;
    i32 y;
} v2i;

#define DEFINES_H
#endif
