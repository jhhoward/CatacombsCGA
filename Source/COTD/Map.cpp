#include "Defines.h"
#include "Map.h"
#include "Game.h"
#include "FixedMath.h"
#include "Draw.h"
#include "Platform.h"
#include "Enemy.h"

uint8_t Map::level[MAP_WIDTH * MAP_HEIGHT];
uint8_t Map::roomIndex[MAP_WIDTH * MAP_HEIGHT];

Room Map::rooms[MAX_ROOMS];

bool Map::IsBlocked(uint8_t x, uint8_t y)
{
	return GetCellSafe(x, y) >= CT_FirstCollidableCell;
}

bool Map::IsSolid(uint8_t x, uint8_t y)
{
	return GetCellSafe(x, y) >= CT_FirstSolidCell;
}

CellType Map::GetCell(uint8_t x, uint8_t y) 
{
	int index = y * MAP_WIDTH + x;
	return (CellType) level[index];
}

CellType Map::GetCellSafe(uint8_t x, uint8_t y) 
{
	if(x >= MAP_WIDTH || y >= MAP_HEIGHT)
		return CT_BrickWall;
	
	int index = y * MAP_WIDTH + x;
	return (CellType)level[index];
}

void Map::SetCell(uint8_t x, uint8_t y, CellType type)
{
	if (x >= MAP_WIDTH || y >= MAP_HEIGHT)
	{
		return;
	}

	int index = (y * MAP_WIDTH + x);
	level[index] = (uint8_t) type;

	if (type == CT_BrickWall)
	{
		roomIndex[index] = 0;
	}
}

uint8_t Map::GetRoomIndex(uint8_t x, uint8_t y)
{
	int index = (y * MAP_WIDTH + x);
	return roomIndex[index];
}

void Map::SetRoomIndex(uint8_t x, uint8_t y, uint8_t room)
{
	int index = (y * MAP_WIDTH + x);
	roomIndex[index] = room;
}

Room& Map::GetRoom(uint8_t x, uint8_t y)
{
	int index = (y * MAP_WIDTH + x);
	return rooms[roomIndex[index]];
}

bool Map::IsClearLine(int16_t x1, int16_t y1, int16_t x2, int16_t y2)
{
	int cellX1 = x1 / CELL_SIZE;
	int cellX2 = x2 / CELL_SIZE;
	int cellY1 = y1 / CELL_SIZE;
	int cellY2 = y2 / CELL_SIZE;

    int xdist = ABS(cellX2 - cellX1);

	int partial, delta;
	int deltafrac;
	int xfrac, yfrac;
	int xstep, ystep;
	int32_t ltemp;
	int x, y;

    if (xdist > 0)
    {
        if (cellX2 > cellX1)
        {
            partial = (CELL_SIZE * (cellX1 + 1) - x1);
            xstep = 1;
        }
        else
        {
            partial = (x1 - CELL_SIZE * (cellX1));
            xstep = -1;
        }

        deltafrac = ABS(x2 - x1);
        delta = y2 - y1;
        ltemp = ((int32_t)delta * CELL_SIZE) / deltafrac;
        if (ltemp > 0x7fffl)
            ystep = 0x7fff;
        else if (ltemp < -0x7fffl)
            ystep = -0x7fff;
        else
            ystep = ltemp;
        yfrac = y1 + (((int32_t)ystep*partial) / CELL_SIZE);

        x = cellX1 + xstep;
        cellX2 += xstep;
        do
        {
            y = (yfrac) / CELL_SIZE;
            yfrac += ystep;

			if (IsSolid(x, y))
				return false;

            x += xstep;

            //
            // see if the door is open enough
            //
            /*value &= ~0x80;
            intercept = yfrac-ystep/2;

            if (intercept>doorposition[value])
                return false;*/

        } while (x != cellX2);
    }

    int ydist = ABS(cellY2 - cellY1);

    if (ydist > 0)
    {
        if (cellY2 > cellY1)
        {
            partial = (CELL_SIZE * (cellY1 + 1) - y1);
            ystep = 1;
        }
        else
        {
            partial = (y1 - CELL_SIZE * (cellY1));
            ystep = -1;
        }

        deltafrac = ABS(y2 - y1);
        delta = x2 - x1;
        ltemp = ((int32_t)delta * CELL_SIZE)/deltafrac;
        if (ltemp > 0x7fffl)
            xstep = 0x7fff;
        else if (ltemp < -0x7fffl)
            xstep = -0x7fff;
        else
            xstep = ltemp;
        xfrac = x1 + (((int32_t)xstep*partial) / CELL_SIZE);

        y = cellY1 + ystep;
        cellY2 += ystep;
        do
        {
            x = (xfrac) / CELL_SIZE;
            xfrac += xstep;

			if (IsSolid(x, y))
				return false;
            y += ystep;

            //
            // see if the door is open enough
            //
            /*value &= ~0x80;
            intercept = xfrac-xstep/2;

            if (intercept>doorposition[value])
                return false;*/
        } while (y != cellY2);
    }

    return true;
}

#include <stdio.h>

void Map::GenerateRoomStructure()
{
	Platform::Log("Generating render structures..");

#if USE_GRAPHICS_MODE
#if USE_COMPOSITE_COLOURS
	const uint8_t verticalColour = 0x22;
	const uint8_t horizontalColour = 0x33;
#else
	const uint8_t verticalColour = 0x77;
	const uint8_t horizontalColour = 0x55;
#endif
#elif USE_MONO_OUTPUT
	const uint8_t verticalColour = 7;
	const uint8_t horizontalColour = 7;
#else
	const uint8_t verticalColour = 3;
	const uint8_t horizontalColour = 11;
#endif

	for (int n = 0; n < MAX_ROOMS; n++)
	{
		rooms[n].numVertices = 0;
		rooms[n].numWalls = 0;
	}
	for (int y = 0; y < MAP_HEIGHT; y++)
	{
		for (int x = 0; x < MAP_WIDTH; x++)
		{
			uint8_t index = roomIndex[y * MAP_WIDTH + x];
			if (index != 0)
			{
				Room& room = rooms[index];
				if (GetCell(x, y) != CT_BrickWall)
				{
					if (GetCellSafe(x - 1, y) == CT_BrickWall)
					{
						room.AddWall(x * CELL_SIZE, y * CELL_SIZE + CELL_SIZE, x * CELL_SIZE, y * CELL_SIZE, verticalColour);
					}
					if (GetCellSafe(x, y - 1) == CT_BrickWall)
					{
						room.AddWall(x * CELL_SIZE, y * CELL_SIZE, x * CELL_SIZE + CELL_SIZE, y * CELL_SIZE, horizontalColour);
					}
					if (GetCellSafe(x + 1, y) == CT_BrickWall)
					{
						room.AddWall(x * CELL_SIZE + CELL_SIZE, y * CELL_SIZE, x * CELL_SIZE + CELL_SIZE, y * CELL_SIZE + CELL_SIZE, verticalColour);
					}
					if (GetCellSafe(x, y + 1) == CT_BrickWall)
					{
						room.AddWall(x * CELL_SIZE + CELL_SIZE, y * CELL_SIZE + CELL_SIZE, x * CELL_SIZE, y * CELL_SIZE + CELL_SIZE, horizontalColour);
					}

					uint8_t portalColour = 0;// (room.numWalls & 0xf);
					if (x > 0 && GetCell(x - 1, y) != CT_BrickWall && GetRoomIndex(x - 1, y) != index)
					{
						room.AddWall(x * CELL_SIZE, y * CELL_SIZE + CELL_SIZE, x * CELL_SIZE, y * CELL_SIZE, portalColour, GetRoomIndex(x - 1, y));
					}
					if (y > 0 && GetCell(x, y - 1) != CT_BrickWall && GetRoomIndex(x, y - 1) != index)
					{
						room.AddWall(x * CELL_SIZE, y * CELL_SIZE, x * CELL_SIZE + CELL_SIZE, y * CELL_SIZE, portalColour, GetRoomIndex(x, y - 1));
					}
					if (x < MAP_WIDTH - 1 && GetCell(x + 1, y) != CT_BrickWall && GetRoomIndex(x + 1, y) != index)
					{
						room.AddWall(x * CELL_SIZE + CELL_SIZE, y * CELL_SIZE, x * CELL_SIZE + CELL_SIZE, y * CELL_SIZE + CELL_SIZE, portalColour, GetRoomIndex(x + 1, y));
					}
					if (y < MAP_HEIGHT - 1 && GetCell(x, y + 1) != CT_BrickWall && GetRoomIndex(x, y + 1) != index)
					{
						room.AddWall(x * CELL_SIZE + CELL_SIZE, y * CELL_SIZE + CELL_SIZE, x * CELL_SIZE, y * CELL_SIZE + CELL_SIZE, portalColour, GetRoomIndex(x, y + 1));
					}
				}

				if (GetCell(x, y) == CT_Torch)
				{
					if (Map::IsSolid(x - 1, y))
					{
						room.AddTorch(x * CELL_SIZE + CELL_SIZE / 7, y * CELL_SIZE + CELL_SIZE / 2);
					}
					else if (Map::IsSolid(x + 1, y))
					{
						room.AddTorch(x * CELL_SIZE + 6 * CELL_SIZE / 7, y * CELL_SIZE + CELL_SIZE / 2);
					}
					else if (Map::IsSolid(x, y - 1))
					{
						room.AddTorch(x * CELL_SIZE + CELL_SIZE / 2, y * CELL_SIZE + CELL_SIZE / 7);
					}
					else if (Map::IsSolid(x, y + 1))
					{
						room.AddTorch(x * CELL_SIZE + CELL_SIZE / 2, y * CELL_SIZE + 6 * CELL_SIZE / 7);
					}
				}
			}
		}
	}

	/*
	for (int n = 0; n < MAX_ROOMS; n++)
	{
		Room& room = rooms[n];
		if (room.numWalls > 0)
		{
			printf("Room %d: %d walls, %d verts\n", n, room.numWalls, room.numVertices);
		}
	}
	printf("Starting room: %d\n", GetRoomIndex(1, 1));*/
}

void Room::AddWall(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint8_t colour, uint8_t connectedRoomIndex)
{
	if (numWalls >= MAX_ROOM_WALLS)
	{
		Platform::Log("Too many walls!");
		return;
	}
	int vertA = -1;
	int vertB = -1;
	for (int n = 0; n < numVertices; n++)
	{
		if (vertices[n].x == x1 && vertices[n].y == y1)
		{
			vertA = n;
		}
		if (vertices[n].x == x2 && vertices[n].y == y2)
		{
			vertB = n;
		}
	}

	// Extending wall
	for (int n = 0; n < numWalls; n++)
	{
		if (walls[n].connectedRoomIndex == connectedRoomIndex)
		{
			/*if (walls[n].vertexA == vertA)
			{
				if (vertices[walls[n].vertexB].x == x2 || vertices[walls[n].vertexB].y == y2)
				{
					vertices[vertA].x = x2;
					vertices[vertA].y = y2;
					return;
				}
				break;
			}*/
			if (walls[n].vertexB == vertA)
			{
				if (vertices[walls[n].vertexA].x == x2 || vertices[walls[n].vertexA].y == y2)
				{
					vertices[vertA].x = x2;
					vertices[vertA].y = y2;
					walls[n].length++;
					return;
				}
				break;
			}
			if (walls[n].vertexA == vertB)
			{
				if (vertices[walls[n].vertexB].x == x1 || vertices[walls[n].vertexB].y == y1)
				{
					vertices[vertB].x = x1;
					vertices[vertB].y = y1;
					walls[n].length++;
					return;
				}
				break;
			}
			/*if (walls[n].vertexB == vertB)
			{
				if (vertices[walls[n].vertexA].x == x1 || vertices[walls[n].vertexA].y == y1)
				{
					vertices[vertB].x = x1;
					vertices[vertB].y = y1;
					return;
				}
				break;
			}*/
		}
	}

	if (vertA == -1)
	{
		if (numVertices >= MAX_ROOM_VERTICES)
		{
			Platform::Log("Too many verts!");
			return;
		}
		vertA = numVertices;
		vertices[vertA].x = x1;
		vertices[vertA].y = y1;
		numVertices++;
	}
	if (vertB == -1)
	{
		if (numVertices >= MAX_ROOM_VERTICES)
		{
			Platform::Log("Too many verts!");
			return;
		}
		vertB = numVertices;
		vertices[vertB].x = x2;
		vertices[vertB].y = y2;
		numVertices++;
	}

	walls[numWalls].Set(vertA, vertB, connectedRoomIndex, colour);
	numWalls++;
}

void Room::AddTorch(int16_t x, int16_t y)
{
	if (numTorches < MAX_ROOM_TORCHES)
	{
		torches[numTorches].x = x;
		torches[numTorches].y = y;
		numTorches++;
	}
}

#if WITH_DOORS
void Door::Set(int x, int y, bool inHorizontal)
{
	isActive = true;
	isHorizontal = inHorizontal;

	if (isHorizontal)
	{
		hingeLeft.x = x * CELL_SIZE;
		hingeLeft.y = y * CELL_SIZE + CELL_SIZE / 2;
		hingeRight.x = x * CELL_SIZE + CELL_SIZE;
		hingeRight.y = y * CELL_SIZE + CELL_SIZE / 2;
	}
	else
	{
		hingeLeft.x = x * CELL_SIZE + CELL_SIZE / 2;
		hingeLeft.y = y * CELL_SIZE;
		hingeRight.x = x * CELL_SIZE + CELL_SIZE / 2;
		hingeRight.y = y * CELL_SIZE + CELL_SIZE;
	}
}

void Map::OpenDoor(uint8_t x, uint8_t y)
{
	Room& room = GetRoom(x, y);
	if (room.door.isActive)
	{
		room.door.doorTimer = DOOR_OPEN_TIME;
	}
}
#endif

void Map::Tick()
{
#if WITH_DOORS
	for (int n = 0; n < MAX_ROOMS; n++)
	{
		Room& room = rooms[n];
		if (room.door.isActive)
		{
			if (room.door.doorTimer > 0)
			{
				if (room.door.doorOpen < MAX_DOOR_OPEN)
				{
					room.door.doorOpen += DOOR_OPEN_SPEED;
				}
				else
				{
					room.door.doorTimer--;
				}
			}
			else if (room.door.doorOpen > 0)
			{
				room.door.doorOpen -= DOOR_OPEN_SPEED;
			}
		}
	}
#endif
}
