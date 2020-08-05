#pragma once

#if _WIN32
#include <stdint.h>
#include <string.h>
#define PROGMEM
#define PSTR
#define pgm_read_byte(x) (*((uint8_t*)x))
#define pgm_read_word(x) (*((uint16_t*)x))
#define pgm_read_ptr(x) (*((uintptr_t*)x))
#define strlen_P(x) strlen(x)
#elif _DOS
#include <stdint.h>
#define PROGMEM
#define PSTR
#define nullptr 0
#else
#include <avr/pgmspace.h>
//#define pgm_read_ptr pgm_read_word
#endif

#define USE_MONO_OUTPUT 0
#define USE_LOW_PRECISION_RENDERING 1
#define USE_GRAPHICS_MODE 1
#define USE_COMPOSITE_COLOURS 1

#if _DOS
#define DISPLAY_WIDTH 80
#define DISPLAY_HEIGHT 50

#if USE_GRAPHICS_MODE
typedef unsigned char* backbuffer_t;
#else
typedef unsigned char far* backbuffer_t;
#endif

typedef void (__far* drawRoutine_t)(backbuffer_t backBuffer, unsigned char x, unsigned char scale);

#elif _WIN32
#define DISPLAY_WIDTH 128
#define DISPLAY_HEIGHT 64
#else
#define DISPLAY_WIDTH 128
#define DISPLAY_HEIGHT 64
#endif

#define TARGET_FRAMERATE 30
#define DEV_MODE 0

#define INPUT_LEFT 1
#define INPUT_RIGHT 2
#define INPUT_UP 4
#define INPUT_DOWN 8
#define INPUT_A 16
#define INPUT_B 32

#define COLOUR_WHITE 1
#define COLOUR_BLACK 0

#if USE_MONO_OUTPUT
#define DETAIL_COLOUR 0
#else
#define DETAIL_COLOUR 0
#endif

#define CAMERA_SCALE 1

#if USE_LOW_PRECISION_RENDERING
#define RENDER_PRECISION_SHIFT 3
#define RENDER_ROTATION_PRECISION_SHIFT 2
#define CLIP_PLANE (32 >> RENDER_PRECISION_SHIFT)
#else
#define CLIP_PLANE 32
#endif
#define CLIP_ANGLE 32

#define NEAR_PLANE_MULTIPLIER 130
#define NEAR_PLANE (DISPLAY_WIDTH * NEAR_PLANE_MULTIPLIER / 256)

#define HORIZON (DISPLAY_HEIGHT / 2)

#define CELL_SIZE 256
#define PARTICLES_PER_SYSTEM 8
#define BASE_SPRITE_SIZE 16
#define MAX_SPRITE_SIZE (DISPLAY_HEIGHT / 2)
#define MIN_TEXTURE_DISTANCE 4
#define MAX_QUEUED_DRAWABLES 12

#define TURN_SPEED 3

#define MAX_ROOM_VERTICES 64
#define MAX_ROOMS 64
#define MAX_ROOM_WALLS 64
#define MAX_ROOM_TORCHES 10

#define WITH_DOORS 0

