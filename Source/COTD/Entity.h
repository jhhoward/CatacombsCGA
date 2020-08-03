#pragma once

#include <stdint.h>

class Entity
{
public:
	bool IsOverlappingPoint(int16_t pointX, int16_t pointY) const;
	bool IsOverlappingEntity(const Entity& other) const;

	union
	{
		struct
		{
			uint8_t fracX;
			uint8_t cellX;
		};
		int16_t x;
	};
	union
	{
		struct
		{
			uint8_t fracY;
			uint8_t cellY;
		};
		int16_t y;
	};
	//int16_t x, y;
};
