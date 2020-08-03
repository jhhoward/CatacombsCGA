#include "Platform.h"
#include "MapGenerator.h"
#include "Map.h"
#include "FixedMath.h"
#include "Enemy.h"
#include "Game.h"

uint8_t MapGenerator::nextRoomToGenerate = 0;

uint8_t MapGenerator::GetDistanceToCellType(uint8_t x, uint8_t y, CellType cellType)
{
	uint8_t ringWidth = 3;

	for (uint8_t offset = 1; offset < MAP_WIDTH; offset++)
	{
		for (uint8_t i = 0; i < ringWidth; i++)
		{
			if (Map::GetCellSafe(x - offset + i, y - offset) == cellType)
			{
				return offset;
			}
			if (Map::GetCellSafe(x - offset + i, y + offset) == cellType)
			{
				return offset;
			}
			if (Map::GetCellSafe(x - offset, y - offset + i) == cellType)
			{
				return offset;
			}
			if (Map::GetCellSafe(x + offset, y - offset + i) == cellType)
			{
				return offset;
			}
		}

		ringWidth += 2;
	}

	return 0xff;
}

uint8_t MapGenerator::CountNeighbours(uint8_t x, uint8_t y)
{
	uint8_t result = 0;

	if (Map::GetCellSafe(x + 1, y) == CT_Empty)
		result++;
	if (Map::GetCellSafe(x, y + 1) == CT_Empty)
		result++;
	if (Map::GetCellSafe(x - 1, y) == CT_Empty)
		result++;
	if (Map::GetCellSafe(x, y - 1) == CT_Empty)
		result++;
	if (Map::GetCellSafe(x + 1, y + 1) == CT_Empty)
		result++;
	if (Map::GetCellSafe(x - 1, y + 1) == CT_Empty)
		result++;
	if (Map::GetCellSafe(x - 1, y - 1) == CT_Empty)
		result++;
	if (Map::GetCellSafe(x + 1, y - 1) == CT_Empty)
		result++;

	return result;
}

MapGenerator::NeighbourInfo MapGenerator::GetCellNeighbourInfo(uint8_t x, uint8_t y)
{
	NeighbourInfo result;

	result.count = 0;
	result.mask = 0;

	if (Map::IsSolid(x, y - 1))
	{
		result.hasNorth = true;
		result.count++;
	}
	if (Map::IsSolid(x + 1, y))
	{
		result.hasEast = true;
		result.count++;
	}
	if (Map::IsSolid(x, y + 1))
	{
		result.hasSouth = true;
		result.count++;
	}
	if (Map::IsSolid(x - 1, y))
	{
		result.hasWest = true;
		result.count++;
	}

	return result;
}

uint8_t MapGenerator::CountImmediateNeighbours(uint8_t x, uint8_t y)
{
	uint8_t result = 0;

	if (Map::GetCellSafe(x + 1, y) == CT_Empty)
		result++;
	if (Map::GetCellSafe(x, y + 1) == CT_Empty)
		result++;
	if (Map::GetCellSafe(x - 1, y) == CT_Empty)
		result++;
	if (Map::GetCellSafe(x, y - 1) == CT_Empty)
		result++;

	return result;
}

MapGenerator::NeighbourInfo MapGenerator::GetRoomNeighbourMask(uint8_t x, uint8_t y, uint8_t w, uint8_t h)
{
	NeighbourInfo result;
	result.mask = 0;
	result.count = 0;

	result.canDemolishNorth = y > 1;
	result.canDemolishWest = x > 1;
	result.canDemolishEast = x + w + 1 < MAP_WIDTH - 1;
	result.canDemolishSouth = y + h + 1 < MAP_HEIGHT - 1;

	// Don't demolish walls if the neighbouring room has the same wall length
	if (Map::GetCell(x - 1, y - 2) != CT_Empty && Map::GetCell(x + w, y - 2) != CT_Empty)
	{
		result.canDemolishNorth = false;
	}
	if (Map::GetCell(x - 2, y - 1) != CT_Empty && Map::GetCell(x - 2, y + h) != CT_Empty)
	{
		result.canDemolishWest = false;
	}
	if (Map::GetCell(x + w + 1, y - 1) != CT_Empty && Map::GetCell(x + w, y + h + 1) != CT_Empty)
	{
		result.canDemolishEast = false;
	}
	if (Map::GetCell(x - 1, y + h + 1) != CT_Empty && Map::GetCell(x + w, y + h + 1) != CT_Empty)
	{
		result.canDemolishSouth = false;
	}

	// Don't demolish wall if this will leave an unattached wall
	if (Map::GetCell(x - 1, y - 2) == CT_Empty && Map::GetCell(x - 2, y - 1) == CT_Empty)
	{
		result.canDemolishNorth = false;
		result.canDemolishWest = false;
	}
	if (Map::GetCell(x + w, y - 2) == CT_Empty && Map::GetCell(x + w + 1, y - 1) == CT_Empty)
	{
		result.canDemolishNorth = false;
		result.canDemolishEast = false;
	}
	if (Map::GetCell(x + w, y + h + 1) == CT_Empty && Map::GetCell(x + w + 1, y + h) == CT_Empty)
	{
		result.canDemolishSouth = false;
		result.canDemolishEast = false;
	}
	if (Map::GetCell(x - 1, y + h + 1) == CT_Empty && Map::GetCell(x - 2, y + h) == CT_Empty)
	{
		result.canDemolishSouth = false;
		result.canDemolishWest = false;
	}

	bool hasNorthWall = Map::GetCell(x, y - 1) != CT_Empty && Map::GetCell(x + w - 1, y - 1) != CT_Empty;
	bool hasEastWall = Map::GetCell(x + w, y) != CT_Empty && Map::GetCell(x + w, y + h - 1) != CT_Empty;
	bool hasSouthWall = Map::GetCell(x, y + h) != CT_Empty && Map::GetCell(x + w - 1, y + h) != CT_Empty;
	bool hasWestWall = Map::GetCell(x - 1, y) != CT_Empty && Map::GetCell(x - 1, y + h - 1) != CT_Empty;

	if (!hasNorthWall)
	{
		result.canDemolishNorth = false;
		result.canDemolishEast = false;
		result.canDemolishWest = false;
	}
	if (!hasEastWall)
	{
		result.canDemolishNorth = false;
		result.canDemolishEast = false;
		result.canDemolishSouth = false;
	}
	if (!hasSouthWall)
	{
		result.canDemolishEast = false;
		result.canDemolishSouth = false;
		result.canDemolishWest = false;
	}
	if (!hasWestWall)
	{
		result.canDemolishNorth = false;
		result.canDemolishSouth = false;
		result.canDemolishWest = false;
	}

	for (int i = x; i < x + w; i++)
	{
		if (Map::GetCell(i, y - 1) == CT_Empty)
		{
			result.hasNorth = true;
			result.count++;
		}
		if (Map::GetCell(i, y + h) == CT_Empty)
		{
			result.hasSouth = true;
			result.count++;
		}

		// Don't demolish wall if there is an intersecting wall attached
		if (y > 1 && Map::GetCell(i, y - 2) != CT_Empty)
		{
			result.canDemolishNorth = false;
		}
		if (y + h + 1 < MAP_HEIGHT - 1 && Map::GetCell(i, y + h + 1) != CT_Empty)
		{
			result.canDemolishSouth = false;
		}
	}
	for (int j = y; j < y + h; j++)
	{
		if (Map::GetCell(x - 1, j) == CT_Empty)
		{
			result.hasWest = true;
			result.count++;
		}
		if (Map::GetCell(x + w, j) == CT_Empty)
		{
			result.hasEast = true;
			result.count++;
		}

		// Don't demolish wall if there is an intersecting wall attached
		if (x > 1 && Map::GetCell(x - 2, j) != CT_Empty)
		{
			result.canDemolishWest = false;
		}
		if (x + w + 1 < MAP_WIDTH - 1 && Map::GetCell(x + w + 1, j) != CT_Empty)
		{
			result.canDemolishEast = false;
		}
	}

	return result;
}

void MapGenerator::SplitMap(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t doorX, uint8_t doorY)
{
	const int minRoomSize = 3;
	const int maxRoomSize = 8;
	const int maxFloorSpace = 80;
	const int demolishWallChance = 20;

	if (doorX != 0 && doorY != 0)
	{
		if (Map::GetCell(doorX, doorY) == CT_BrickWall)
		{
			uint8_t doorRoomIndex = nextRoomToGenerate;
			nextRoomToGenerate++;
			Map::SetCell(doorX, doorY, CT_Empty);
			Map::SetRoomIndex(doorX, doorY, doorRoomIndex);

#if WITH_DOORS
			bool isHorizontal = Map::GetCell(doorX - 1, doorY) == CT_BrickWall;
			Map::GetRoom(doorRoomIndex).door.Set(doorX, doorY, isHorizontal);
#endif
		}
	}

	bool splitVertical = false;
	bool splitHorizontal = false;

	if (w > maxRoomSize || h > maxRoomSize)//w * h > maxFloorSpace)
	{
		if (w < h)
		{
			splitVertical = true;
		}
		else
		{
			splitHorizontal = true;
		}
	}

	if (splitVertical)
	{
		uint8_t splitSize;
		uint8_t splitAttempts = 255;
		do
		{
			splitSize = (Random() % (h - 2 * minRoomSize)) + minRoomSize;
			splitAttempts--;
		} while (splitAttempts > 0 && (Map::GetCell(x - 1, y + splitSize) == CT_Empty || Map::GetCell(x + w, y + splitSize) == CT_Empty
			|| Map::GetCell(x - 1, y + splitSize - 1) == CT_Empty || Map::GetCell(x + w, y + splitSize - 1) == CT_Empty
			|| Map::GetCell(x - 1, y + splitSize + 1) == CT_Empty || Map::GetCell(x + w, y + splitSize + 1) == CT_Empty));

		if (splitAttempts > 0)
		{
			uint8_t splitDoorX = x + (Random() % (w - 2)) + 1;
			uint8_t splitDoorY = y + splitSize;

			for (uint8_t i = x; i < x + w; i++)
			{
				Map::SetCell(i, y + splitSize, CT_BrickWall);
			}

			uint8_t newRoomIndex = nextRoomToGenerate;
			nextRoomToGenerate++;

			for (uint8_t j = y; j < y + splitSize; j++)
			{
				for (uint8_t i = x; i < x + w; i++)
				{
					Map::SetRoomIndex(i, j, newRoomIndex);
				}
			}

			SplitMap(x, y + splitSize + 1, w, h - splitSize - 1, splitDoorX, splitDoorY);
			SplitMap(x, y, w, splitSize, splitDoorX, splitDoorY);
			return;
		}
	}
	else if (splitHorizontal)
	{
		uint8_t splitSize;
		uint8_t splitAttempts = 255;
		do
		{
			splitSize = (Random() % (w - 2 * minRoomSize)) + minRoomSize;
			splitAttempts--;
		} while (splitAttempts > 0 && (Map::GetCell(x + splitSize, y - 1) == CT_Empty || Map::GetCell(x + splitSize, y + h) == CT_Empty
			|| Map::GetCell(x + splitSize - 1, y - 1) == CT_Empty || Map::GetCell(x + splitSize - 1, y + h) == CT_Empty
			|| Map::GetCell(x + splitSize + 1, y - 1) == CT_Empty || Map::GetCell(x + splitSize + 1, y + h) == CT_Empty));

		if (splitAttempts > 0)
		{
			uint8_t splitDoorX = x + splitSize;
			uint8_t splitDoorY = y + (Random() % (h - 2)) + 1;

			for (uint8_t j = y; j < y + h; j++)
			{
				Map::SetCell(x + splitSize, j, CT_BrickWall);
			}

			uint8_t newRoomIndex = nextRoomToGenerate;
			nextRoomToGenerate++;

			for (uint8_t j = y; j < y + h; j++)
			{
				for (uint8_t i = x; i < x + splitSize; i++)
				{
					Map::SetRoomIndex(i, j, newRoomIndex);
				}
			}

			SplitMap(x + splitSize + 1, y, w - splitSize - 1, h, splitDoorX, splitDoorY);
			SplitMap(x, y, splitSize, h, splitDoorX, splitDoorY);
			return;
		}
	}

	{
		NeighbourInfo neighbours = GetRoomNeighbourMask(x, y, w, h);
		uint8_t roomIndex = Map::GetRoomIndex(x, y);

		if (neighbours.canDemolishNorth && (Random() % 100) < demolishWallChance)
		{
			for (int i = 0; i < w; i++)
			{
				Map::SetCell(x + i, y - 1, CT_Empty);
				Map::SetRoomIndex(x + i, y - 1, roomIndex);
			}
		}
		else if (neighbours.canDemolishWest && (Random() % 100) < demolishWallChance)
		{
			for (int j = 0; j < h; j++)
			{
				Map::SetCell(x - 1, y + j, CT_Empty);
				Map::SetRoomIndex(x - 1, y + j, roomIndex);
			}
		}
		else if (neighbours.canDemolishSouth && (Random() % 100) < demolishWallChance)
		{
			for (int i = 0; i < w; i++)
			{
				Map::SetCell(x + i, y + h, CT_Empty);
				Map::SetRoomIndex(x + i, y + h, roomIndex);
			}
		}
		else if (neighbours.canDemolishEast && (Random() % 100) < demolishWallChance)
		{
			for (int j = 0; j < h; j++)
			{
				Map::SetCell(x + w, y + j, CT_Empty);
				Map::SetRoomIndex(x + w, y + j, roomIndex);
			}
		}

		// Add decorations
		{
			// Add four cornering columns
			if (w == h && w >= 7 && h >= 7)
			{
				Map::SetCell(x + 1, y + 1, CT_BrickWall);
				Map::SetCell(x + w - 2, y + 1, CT_BrickWall);
				Map::SetCell(x + w - 2, y + h - 2, CT_BrickWall);
				Map::SetCell(x + 1, y + h - 2, CT_BrickWall);
			}
		}
	}
}

#include <stdio.h>

void MapGenerator::Generate()
{
	uint8_t playerStartX = 1;
	uint8_t playerStartY = 1;

	Platform::Log("Generating level..");
	fflush(stdout);

	for (int y = 0; y < MAP_HEIGHT; y++)
	{
		for (int x = 0; x < MAP_WIDTH; x++)
		{
			bool isEdge = x == 0 || y == 0 || x == MAP_WIDTH - 1 || y == MAP_HEIGHT - 1;
			Map::SetCell(x, y, isEdge ? CT_BrickWall : CT_Empty);
			Map::SetRoomIndex(x, y, isEdge ? 0 : 1);
		}
	}
	nextRoomToGenerate = 2;

	SplitMap(1, 1, MAP_WIDTH - 2, MAP_HEIGHT - 2, 0, 0);

	// Find any big open spaces
	{
		bool hasOpenSpaces = true;

		while (hasOpenSpaces)
		{
			hasOpenSpaces = false;

			uint8_t x = 0, y = 0, space = 0;

			for (uint8_t i = 1; i < MAP_WIDTH - 1; i++)
			{
				for (uint8_t j = 0; j < MAP_HEIGHT - 1; j++)
				{
					bool foundWall = false;

					for (uint8_t k = 0; k < MAP_HEIGHT && !foundWall; k++)
					{
						for (uint8_t u = 0; u < k && !foundWall; u++)
						{
							for (uint8_t v = 0; v < k && !foundWall; v++)
							{
								if (Map::GetCellSafe(i + u, j + v) != CT_Empty)
								{
									foundWall = true;
								}
							}
						}

						if (!foundWall && k > space)
						{
							space = k;
							x = i;
							y = j;
						}
					}
				}
			}

			if (space > 6)
			{
				hasOpenSpaces = true;

				// Stick a donut in the middle
				for (uint8_t j = 2; j < space - 2; j++)
				{
					for (uint8_t i = 2; i < space - 2; i++)
					{
						Map::SetCell(x + i, y + j, CT_BrickWall);
					}
				}
				/*for (uint8_t n = 2; n < space - 2; n++)
				{
					Map::SetCell(x + n, y + 2, CT_BrickWall);
					Map::SetCell(x + 2, y + n, CT_BrickWall);
					Map::SetCell(x + n, y + space - 3, CT_BrickWall);
					Map::SetCell(x + space - 3, y + n, CT_BrickWall);
				}*/
			}
		}
	}

#if 0
	// Add torches
	{
		uint8_t attempts = 255;
		uint8_t toSpawn = 64;
		uint8_t minSpacing = 3;

		while (attempts > 0 && toSpawn > 0)
		{
			uint8_t x = Random() % MAP_WIDTH;
			uint8_t y = Random() % MAP_HEIGHT;

			if (Map::GetCellSafe(x, y) == CT_Empty)
			{
				NeighbourInfo info = GetCellNeighbourInfo(x, y);

				if(info.count == 1 && GetDistanceToCellType(x, y, CT_Torch) > minSpacing)
				{
					Map::SetCell(x, y, CT_Torch);
					toSpawn--;
					attempts = 255;
				}
			}

			attempts--;
		}
	}

	// Add monsters
	{
		uint8_t attempts = 255;
		uint8_t monstersToSpawn = MAX_ENEMIES;
		CellType monsterType = CT_Monster;
		uint8_t minSpacing = 3;

		while (attempts > 0 && monstersToSpawn > 0)
		{
			uint8_t x = Random() % MAP_WIDTH;
			uint8_t y = Random() % MAP_HEIGHT;

			if (Map::GetCellSafe(x, y) == CT_Empty && Map::IsClearLine(x * CELL_SIZE + CELL_SIZE / 2, y * CELL_SIZE + CELL_SIZE / 2, playerStartX * CELL_SIZE + CELL_SIZE / 2, playerStartY * CELL_SIZE + CELL_SIZE / 2) == false)
			{
				NeighbourInfo info = GetCellNeighbourInfo(x, y);
				if (info.count == 0 && GetDistanceToCellType(x, y, monsterType) > minSpacing)
				{
					Map::SetCell(x, y, monsterType);
					monstersToSpawn--;
					attempts = 255;
				}
			}

			attempts--;
		}
	}
#endif

	// Add blocking decorations
	{
		uint8_t attempts = 255;
		uint8_t toSpawn = 255;
		CellType cellType = CT_Urn;
		uint8_t minSpacing = 3;

		while (attempts > 0 && toSpawn > 0)
		{
			uint8_t x = Random() % MAP_WIDTH;
			uint8_t y = Random() % MAP_HEIGHT;

			if (Map::GetCellSafe(x, y) == CT_Empty)
			{
				NeighbourInfo info = GetCellNeighbourInfo(x, y);

				if(info.IsCorner() && GetDistanceToCellType(x, y, cellType) > minSpacing)
				{
					Map::SetCell(x, y, cellType);
					toSpawn--;
					attempts = 255;
				}
			}

			attempts--;
		}
	}

#if 0
	// Add entrance and exit
	Map::SetCell(1, 1, CT_Entrance);
	Map::SetCell(MAP_WIDTH - 3, MAP_HEIGHT - 3, CT_Exit);

	// Add sign
	if(false)
	{
		uint16_t attempts = 65535;
		const uint8_t closeness = 5;

		while (attempts > 0)
		{
			uint8_t x = Random() % closeness;
			uint8_t y = Random() % closeness;

			if (Map::GetCellSafe(x, y) == CT_Empty
				&&	Map::GetCellSafe(x - 1, y) == CT_Empty
				&&	Map::GetCellSafe(x, y - 1) == CT_Empty
				&&	Map::GetCellSafe(x + 1, y) == CT_Empty
				&&	Map::GetCellSafe(x, y + 1) == CT_Empty
				&&	Map::GetCellSafe(x - 1, y - 1) == CT_Empty
				&&	Map::GetCellSafe(x + 1, y - 1) == CT_Empty
				&&	Map::GetCellSafe(x - 1, y + 1) == CT_Empty
				&&	Map::GetCellSafe(x + 1, y + 1) == CT_Empty
				&& Map::IsClearLine(x * CELL_SIZE + CELL_SIZE / 2, y * CELL_SIZE + CELL_SIZE / 2, playerStartX * CELL_SIZE + CELL_SIZE / 2, playerStartY * CELL_SIZE + CELL_SIZE / 2))
			{
				Map::SetCell(x, y, CT_Sign);
				break;
			}

			attempts--;
		}
	}
	else if(Game::floor == 1)
	{
		Map::SetCell(2, 2, CT_Sign);
	}
	
	// Add treasure / items
	{
		uint16_t attempts = 65535;
		uint8_t toSpawn = 8;
		CellType cellType = CT_Chest;
		uint8_t minSpacing = 3;
		uint8_t minExitSpacing = 6;

		while (attempts > 0 && toSpawn > 0)
		{
			uint8_t x = Random() % MAP_WIDTH;
			uint8_t y = Random() % MAP_HEIGHT;

			switch (Random() % 5)
			{
			case 0:
				cellType = CT_Potion;
				break;
			case 1:
				cellType = CT_Coins;
				break;
			case 2:
				cellType = CT_Chest;
				break;
			case 3:
				cellType = CT_Crown;
				break;
			case 4:
				cellType = CT_Scroll;
				break;
			}

			if (Map::GetCellSafe(x, y) == CT_Empty)
			{
				NeighbourInfo info = GetCellNeighbourInfo(x, y);

				if(info.count == 1 
				&& GetDistanceToCellType(x, y, cellType) > minSpacing
				&& GetDistanceToCellType(x, y, CT_Entrance) > minExitSpacing
				&& GetDistanceToCellType(x, y, CT_Exit) > minExitSpacing)
				{
					Map::SetCell(x, y, cellType);
					toSpawn--;
					attempts = 255;
				}
			}

			attempts--;
		}
	}
#endif

	Map::GenerateRoomStructure();

	DumpMap();
}

#include <stdio.h>

uint8_t rgb[257*3] =
{
	0xff, 0xff, 0xff
};

void MapGenerator::DumpMap()
{
	FILE* fs = fopen("map.dat", "wb");

	for (int n = 0; n < 256; n++)
	{
		rgb[n * 3 + 3] = (n & 3) << 6;
		rgb[n * 3 + 4] = (n & 0x1c) << 3;
		rgb[n * 3 + 5] = (n & 0xe0);
	}

	for (int y = 0; y < MAP_HEIGHT; y++)
	{
		for (int x = 0; x < MAP_WIDTH; x++)
		{
			CellType cellType = Map::GetCell(x, y);
			if (cellType == CT_BrickWall)
			{
				fwrite(rgb, 1, 3, fs);
			}
			else 
			{
				uint8_t room = Map::GetRoomIndex(x, y);
				fwrite(rgb + 3 + 3 * room, 1, 3, fs);
			}
		}
	}

	fclose(fs);
}
