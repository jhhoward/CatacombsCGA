#pragma once

#include <stdint.h>
#include "Defines.h"

#define MAP_WIDTH 32
#define MAP_HEIGHT 24

enum CellType 
{
	CT_Empty = 0,

	// Monster types
	CT_Monster,

	// Non collidable decorations
	CT_Torch,
	CT_Entrance,
	CT_Exit,

	// Items
	CT_Potion,
	CT_Coins,
	CT_Crown,
	CT_Scroll,

	// Collidable decorations
	CT_Urn,
	CT_Chest,
	CT_ChestOpened,
	CT_Sign,

	// Solid cells
	CT_BrickWall,
	
	CT_FirstCollidableCell = CT_Urn,
	CT_FirstSolidCell = CT_BrickWall
};

struct Vertex
{
	int16_t x, y;

	void Set(int16_t inX, int16_t inY)
	{
		x = inX;
		y = inY;
	}
};

struct WallSegment
{
	uint8_t vertexA, vertexB;
	uint8_t connectedRoomIndex;
	uint8_t colour;
	uint8_t length;

	void Set(uint8_t a, uint8_t b, uint8_t connected, uint8_t col)
	{
		vertexA = a;
		vertexB = b;
		connectedRoomIndex = connected;
		colour = col;
		length = 1;
	}
};

#if WITH_DOORS
#define MAX_DOOR_OPEN (CELL_SIZE)
#define DOOR_OPEN_TIME 30
#define DOOR_OPEN_SPEED 16

class Door
{
public:
	Door() { isActive = false; doorOpen = 0; }

	bool isActive;
	bool isHorizontal;
	Vertex hingeLeft;
	Vertex hingeRight;
	int16_t doorOpen;
	uint8_t doorTimer;

	void Set(int x, int y, bool inHorizontal);
};
#endif

class Room
{
public:
	int numVertices;
	int numWalls;
	int numTorches;
	Vertex vertices[MAX_ROOM_VERTICES];
	WallSegment walls[MAX_ROOM_WALLS];
	Vertex torches[MAX_ROOM_TORCHES];

#if WITH_DOORS
	Door door;
#endif

	void AddWall(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint8_t colour, uint8_t connectedRoomIndex = 0);
	void AddTorch(int16_t x, int16_t y);
};

class Map
{
public:
	static bool IsSolid(uint8_t x, uint8_t y);
	static bool IsBlocked(uint8_t x, uint8_t y);
	static inline bool IsBlockedAtWorldPosition(int16_t x, int16_t y)
	{
		return IsBlocked((uint8_t)(x >> 8), (uint8_t)(y >> 8));
	}

	static CellType GetCell(uint8_t x, uint8_t y);
	static CellType GetCellSafe(uint8_t x, uint8_t y);
	static void SetCell(uint8_t x, uint8_t y, CellType cellType);

	static uint8_t GetRoomIndex(uint8_t x, uint8_t y);
	static void SetRoomIndex(uint8_t x, uint8_t y, uint8_t room);
	static Room& GetRoom(uint8_t x, uint8_t y);
	static Room& GetRoom(uint8_t index) { return rooms[index]; }

	static bool IsClearLine(int16_t x1, int16_t y1, int16_t x2, int16_t y2);

	static void OpenDoor(uint8_t x, uint8_t y);
	static void Tick();

	static uint8_t roomIndex[MAP_WIDTH * MAP_HEIGHT];

	static void GenerateRoomStructure();

private:	
	static uint8_t level[MAP_WIDTH * MAP_HEIGHT];

	static Room rooms[MAX_ROOMS];
};
