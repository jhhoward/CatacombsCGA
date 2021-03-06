#pragma once

#include <stdint.h>
#if _WIN32
#include <math.h>
#endif
#include "Defines.h"

#define FIXED_SHIFT 8
#define FIXED_ONE (1 << FIXED_SHIFT)
#define INT_TO_FIXED(x) ((x) * FIXED_ONE)
#define FIXED_TO_INT(x) ((x) >> 8)
#define FLOAT_TO_FIXED(x) ((int16_t)((x) * FIXED_ONE))

#define ABS(x) (((x) < 0) ? -(x) : (x))

#define FIXED_ANGLE_MAX 1024
#define FIXED_ANGLE_WRAP(x) ((x) & 1023)
#define FIXED_ANGLE_45 (FIXED_ANGLE_90 / 2)
#define FIXED_ANGLE_90 (FIXED_ANGLE_MAX / 4)
#define FIXED_ANGLE_180 (FIXED_ANGLE_90 * 2)
#define FIXED_ANGLE_270 (FIXED_ANGLE_90 * 3)
#define FIXED_ANGLE_TO_RADIANS(x) ((x) * (2.0f * 3.141592654f / FIXED_ANGLE_MAX))

typedef uint16_t angle_t;
extern const int16_t sinTable[FIXED_ANGLE_MAX];

inline int16_t FixedSin(angle_t angle)
{
	return sinTable[FIXED_ANGLE_WRAP(angle)];
}

inline int16_t FixedCos(angle_t angle)
{
	return sinTable[FIXED_ANGLE_WRAP(FIXED_ANGLE_90 - angle)];
}

uint16_t Random();
void SeedRandom(uint16_t seed);

