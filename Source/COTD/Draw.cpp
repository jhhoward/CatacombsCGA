#include <stdint.h>
#include <stdio.h>
#include <memory.h>
#include <dos.h>
#include <i86.h>
#include <malloc.h>
#include "Draw.h"
#include "Defines.h"
#include "Game.h"
#include "Particle.h"
#include "FixedMath.h"
#include "Map.h"
#include "Projectile.h"
#include "Platform.h"
#include "Enemy.h"
//#include "Font.h"
#include "DOSLib.h"
#include "Profiler.h"

#include "Generated/LUT.inc.h"
#include "Generated/WallScaler.inc.h"
#include "Generated/DrawRoutines.inc.h"
#include "Generated/SpriteData.inc.h"
//#include "Generated/handsprite.h"
//#include "Generated/handsprite2.h"

Camera Renderer::camera;
uint8_t Renderer::wBuffer[DISPLAY_WIDTH];
uint8_t Renderer::wallId[DISPLAY_WIDTH];
uint8_t Renderer::wallColourUpper[DISPLAY_WIDTH];
uint8_t Renderer::wallColourLower[DISPLAY_WIDTH];
int8_t Renderer::horizonBuffer[DISPLAY_WIDTH];
uint8_t Renderer::globalRenderFrame = 0;
uint8_t Renderer::numBufferSlicesFilled = 0;
QueuedDrawable Renderer::queuedDrawables[MAX_QUEUED_DRAWABLES];
uint8_t Renderer::numQueuedDrawables = 0;
uint8_t Renderer::currentWallId = 0;
bool Renderer::visibleRooms[MAX_ROOMS];

static int wallDrawCounter = 0;
static int wallSegmentDrawCounter = 0;
static int cellDrawCounter = 0;
static int visibleSegmentCounter = 0;

void inline Renderer::DrawWallSegment(RoomDrawContext& context, int16_t x1, int16_t w1, int16_t x2, int16_t w2, uint8_t colour)
{
	wallSegmentDrawCounter++;

	int16_t dx;
	int16_t werror;
	int16_t w;
	int16_t dw;
	int8_t wstep;
	int x;

	if (x1 < context.clipLeft)
	{
#if USE_LOW_PRECISION_RENDERING
		int16_t xClipDiff = context.clipLeft - x1;
		int16_t xDiff = x2 - x1;
		int16_t wDiff = w2 - w1;

		while (xClipDiff > 128)
		{
			// Drop precision to avoid overflow
			xClipDiff >>= 1;
			xDiff >>= 1;
		}
		w1 += (xClipDiff * wDiff) / xDiff;
#else
		w1 += ((int32_t)(context.clipLeft - x1) * (int32_t)(w2 - w1)) / (x2 - x1);
#endif
		x1 = context.clipLeft;
	}

	dx = x2 - x1;
	werror = dx / 2;

	w = w1;

	if (w1 < w2)
	{
		dw = w2 - w1;
		wstep = 1;
	}
	else
	{
		dw = w1 - w2;
		wstep = -1;
	}

	bool counted = false;

	for (x = x1; x < context.clipRight; x++)
	{
		bool drawSlice = wBuffer[x] < w;

		if (drawSlice)
		{
			if (!counted)
			{
				visibleSegmentCounter++;
				counted = true;
			}
			if (!wBuffer[x])
			{
				numBufferSlicesFilled++;
			}

			uint8_t outW = (uint8_t)(w);

			if (outW > 63)
				wBuffer[x] = 63;
			else
				wBuffer[x] = outW;

			wallId[x] = currentWallId;
			wallColourUpper[x] = colour;
			wallColourLower[x] = colour;
		}

		if (x == x2)
			break;

		werror -= dw;

		while (werror < 0)
		{
			w += wstep;
			werror += dx;
		}
	}
}

inline bool Renderer::isFrustrumClipped(int16_t x, int16_t y)
{
	if ((camera.clipCos * (x - camera.cellX) - camera.clipSin * (y - camera.cellY)) < -512)
		return true;
	if ((camera.clipSin * (x - camera.cellX) + camera.clipCos * (y - camera.cellY)) < -512)
		return true;

	return false;
}

inline void Renderer::TransformToViewSpace(int16_t x, int16_t y, int16_t& outX, int16_t& outY)
{
#if USE_LOW_PRECISION_RENDERING
	int16_t relX = (x - camera.x) >> RENDER_PRECISION_SHIFT;
	int16_t relY = (y - camera.y) >> RENDER_PRECISION_SHIFT;
	outY = (int16_t)((camera.rotCos * relX) >> (8 - RENDER_ROTATION_PRECISION_SHIFT)) - (int16_t)((camera.rotSin * relY) >> (8 - RENDER_ROTATION_PRECISION_SHIFT));
	outX = (int16_t)((camera.rotSin * relX) >> (8 - RENDER_ROTATION_PRECISION_SHIFT)) + (int16_t)((camera.rotCos * relY) >> (8 - RENDER_ROTATION_PRECISION_SHIFT));
#else
	int32_t relX = x - camera.x;
	int32_t relY = y - camera.y;
	outY = (int16_t)((camera.rotCos * relX) >> 8) - (int16_t)((camera.rotSin * relY) >> 8);
	outX = (int16_t)((camera.rotSin * relX) >> 8) + (int16_t)((camera.rotCos * relY) >> 8);
#endif
}

inline void Renderer::TransformToScreenSpace(int16_t viewX, int16_t viewZ, int16_t& outX, int16_t& outW)
{
	// apply perspective projection
#if USE_LOW_PRECISION_RENDERING
	outX = (viewX * NEAR_PLANE * CAMERA_SCALE / viewZ);
#else
	outX = (int16_t)((int32_t)viewX * NEAR_PLANE * CAMERA_SCALE / viewZ);
#endif

#if USE_LOW_PRECISION_RENDERING
	outW = (int16_t)((CELL_SIZE / (2 << RENDER_PRECISION_SHIFT) * NEAR_PLANE * CAMERA_SCALE) / viewZ);
#else
	outW = (int16_t)((CELL_SIZE / 2 * NEAR_PLANE * CAMERA_SCALE) / viewZ);
#endif

	// transform into screen space
	outX = (int16_t)((DISPLAY_WIDTH / 2) + outX);
}

void Renderer::DrawCell(uint8_t x, uint8_t y)
{
	CellType cellType = Map::GetCellSafe(x, y);

	if (isFrustrumClipped(x, y))
	{
		return;
	}

#if 0
	switch (cellType)
	{
	case CellType::Torch:
	{
		const uint16_t* torchSpriteData = Game::globalTickFrame & 4 ? torchSpriteData1 : torchSpriteData2;
		constexpr uint8_t torchScale = 75;

		if (Map::IsSolid(x - 1, y))
		{
			DrawObject(torchSpriteData, x * CELL_SIZE + CELL_SIZE / 7, y * CELL_SIZE + CELL_SIZE / 2, torchScale, AnchorType::Center);
		}
		else if (Map::IsSolid(x + 1, y))
		{
			DrawObject(torchSpriteData, x * CELL_SIZE + 6 * CELL_SIZE / 7, y * CELL_SIZE + CELL_SIZE / 2, torchScale, AnchorType::Center);
		}
		else if (Map::IsSolid(x, y - 1))
		{
			DrawObject(torchSpriteData, x * CELL_SIZE + CELL_SIZE / 2, y * CELL_SIZE + CELL_SIZE / 7, torchScale, AnchorType::Center);
		}
		else if (Map::IsSolid(x, y + 1))
		{
			DrawObject(torchSpriteData, x * CELL_SIZE + CELL_SIZE / 2, y * CELL_SIZE + 6 * CELL_SIZE / 7, torchScale, AnchorType::Center);
		}
	}
	return;
	case CellType::Entrance:
		DrawObject(entranceSpriteData, x * CELL_SIZE + CELL_SIZE / 2, y * CELL_SIZE + CELL_SIZE / 2, 96, AnchorType::Ceiling);
		return;
	case CellType::Exit:
		DrawObject(exitSpriteData, x * CELL_SIZE + CELL_SIZE / 2, y * CELL_SIZE + CELL_SIZE / 2, 96);
		return;
	case CellType::Urn:
		DrawObject(urnSpriteData, x * CELL_SIZE + CELL_SIZE / 2, y * CELL_SIZE + CELL_SIZE / 2, 80);
		return;
	case CellType::Potion:
		DrawObject(potionSpriteData, x * CELL_SIZE + CELL_SIZE / 2, y * CELL_SIZE + CELL_SIZE / 2, 64);
		return;
	case CellType::Scroll:
		DrawObject(scrollSpriteData, x * CELL_SIZE + CELL_SIZE / 2, y * CELL_SIZE + CELL_SIZE / 2, 64);
		return;
	case CellType::Coins:
		DrawObject(coinsSpriteData, x * CELL_SIZE + CELL_SIZE / 2, y * CELL_SIZE + CELL_SIZE / 2, 64);
		return;
	case CellType::Crown:
		DrawObject(crownSpriteData, x * CELL_SIZE + CELL_SIZE / 2, y * CELL_SIZE + CELL_SIZE / 2, 64);
		return;
	case CellType::Sign:
		DrawObject(signSpriteData, x * CELL_SIZE + CELL_SIZE / 2, y * CELL_SIZE + CELL_SIZE / 2, 80);
		return;
	case CellType::Chest:
		DrawObject(chestSpriteData, x * CELL_SIZE + CELL_SIZE / 2, y * CELL_SIZE + CELL_SIZE / 2, 75);
		return;
	case CellType::ChestOpened:
		DrawObject(chestOpenSpriteData, x * CELL_SIZE + CELL_SIZE / 2, y * CELL_SIZE + CELL_SIZE / 2, 75);
		return;
	default:
		break;
	}
#endif

	if(numBufferSlicesFilled >= DISPLAY_WIDTH)
	{
		return;
	}

	if (!Map::IsSolid(x, y))
	{
		return;
	}

	cellDrawCounter++;

	int16_t x1 = x * CELL_SIZE;
	int16_t y1 = y * CELL_SIZE;
	int16_t x2 = x1 + CELL_SIZE;
	int16_t y2 = y1 + CELL_SIZE;

	bool blockedLeft = Map::IsSolid(x - 1, y);
	bool blockedRight = Map::IsSolid(x + 1, y);
	bool blockedUp = Map::IsSolid(x, y - 1);
	bool blockedDown = Map::IsSolid(x, y + 1);

#if 0
	if (!blockedLeft && camera.x < x1)
	{
		DrawWall(x1, y1, x1, y2, 3, !blockedUp && camera.y > y1 ? Edge_Line : Edge_Brick, !blockedDown && camera.y < y2 ? Edge_Line : Edge_None);
	}

	if (!blockedDown && camera.y > y2)
	{
		DrawWall(x1, y2, x2, y2, 11, !blockedLeft && camera.x > x1 ? Edge_Line : Edge_Brick, !blockedRight && camera.x < x2 ? Edge_Line : Edge_None);
	}

	if (!blockedRight && camera.x > x2)
	{
		DrawWall(x2, y2, x2, y1, 3, !blockedDown && camera.y < y2 ? Edge_Line : Edge_Brick, !blockedUp && camera.y > y1 ? Edge_Line : Edge_None);
	}

	if (!blockedUp && camera.y < y1)
	{
		DrawWall(x2, y1, x1, y1, 11, !blockedRight && camera.x < x2 ? Edge_Line : Edge_Brick, !blockedLeft && camera.x > x1 ? Edge_Line : Edge_None);
	}
#endif
}

#define MAP_BUFFER_WIDTH 12  // 16
#define MAP_BUFFER_HEIGHT 12 // 16
#define MAP_BUFFER_LOOKAHEAD 5 //7

void Renderer::DrawCells()
{
	int16_t cosAngle = FixedCos(camera.angle);
	int16_t sinAngle = FixedSin(camera.angle);

	int8_t bufferX = (int8_t)((camera.x + cosAngle * MAP_BUFFER_LOOKAHEAD) >> 8) - MAP_BUFFER_WIDTH / 2;
	int8_t bufferY = (int8_t)((camera.y + sinAngle * MAP_BUFFER_LOOKAHEAD) >> 8) - MAP_BUFFER_WIDTH / 2;; 
	
	if(bufferX < 0)
		bufferX = 0;
	if(bufferY < 0)
		bufferY = 0;
	if(bufferX > MAP_WIDTH - MAP_BUFFER_WIDTH)
		bufferX = MAP_WIDTH - MAP_BUFFER_WIDTH;
	if(bufferY > MAP_HEIGHT - MAP_BUFFER_HEIGHT)
		bufferY = MAP_HEIGHT - MAP_BUFFER_HEIGHT;
	
	// This should make cells draw front to back
	
	int8_t xd, yd;
	int8_t x1, y1, x2, y2;

	if(camera.rotCos > 0)
	{
		x1 = bufferX;
		x2 = x1 + MAP_BUFFER_WIDTH;
		xd = 1;
	}
	else
	{
		x2 = bufferX - 1;
		x1 = x2 + MAP_BUFFER_WIDTH;
		xd = -1;
	}
	if(camera.rotSin < 0)
	{
		y1 = bufferY;
		y2 = y1 + MAP_BUFFER_HEIGHT;
		yd = 1;
	}
	else
	{
		y2 = bufferY - 1;
		y1 = y2 + MAP_BUFFER_HEIGHT;
		yd = -1;
	}

	if(ABS(camera.rotCos) < ABS(camera.rotSin))
	{
		for(int8_t y = y1; y != y2; y += yd)
		{
			for(int8_t x = x1; x != x2; x+= xd)
			{
				DrawCell(x, y);
			}
		}
	}
	else
	{
		for(int8_t x = x1; x != x2; x+= xd)
		{
			for(int8_t y = y1; y != y2; y += yd)
			{
				DrawCell(x, y);
			}
		}
	}	
}

QueuedDrawable* Renderer::CreateQueuedDrawable(uint8_t inverseCameraDistance)
{
	uint8_t insertionPoint = MAX_QUEUED_DRAWABLES;
	
	for(uint8_t n = 0; n < numQueuedDrawables; n++)
	{
		if(inverseCameraDistance < queuedDrawables[n].inverseCameraDistance)
		{
			if(numQueuedDrawables < MAX_QUEUED_DRAWABLES)
			{
				insertionPoint = n;
				numQueuedDrawables++;
				
				for (uint8_t i = numQueuedDrawables - 1; i > n; i--)
				{
					queuedDrawables[i] = queuedDrawables[i - 1];
				}
			}
			else
			{
				if(n == 0)
				{
					// List is full and this is smaller than the first element so just cull
					return nullptr;
				}
				
				// Drop the smallest element to make a space
				for (uint8_t i = 0; i < n - 1; i++)
				{
					queuedDrawables[i] = queuedDrawables[i + 1];
				}
				
				insertionPoint = n - 1;
			}
			
			break;
		}
	}
	
	if(insertionPoint == MAX_QUEUED_DRAWABLES)
	{
		if(numQueuedDrawables < MAX_QUEUED_DRAWABLES)
		{
			insertionPoint = numQueuedDrawables;
			numQueuedDrawables++;
		}
		else if (inverseCameraDistance > queuedDrawables[numQueuedDrawables - 1].inverseCameraDistance)
		{
			// Drop the smallest element to make a space
			for (uint8_t i = 0; i < numQueuedDrawables - 1; i++)
			{
				queuedDrawables[i] = queuedDrawables[i + 1];
			}
			insertionPoint = numQueuedDrawables - 1;
		}
		else
		{
			return nullptr;
		}
	}
	
	return &queuedDrawables[insertionPoint];
}

void inline Renderer::QueueSprite(drawRoutine_t drawRoutine, int8_t x, int8_t y, uint8_t halfSize, uint8_t inverseCameraDistance, bool invert)
{
	if(x < -halfSize * 2)
		return;
	//if(x >= DISPLAY_WIDTH)
	//	return;
	//if(halfSize <= 2)
	//	return;

	QueuedDrawable* drawable = CreateQueuedDrawable(inverseCameraDistance);
	
	if(drawable != nullptr)
	{
		drawable->type = DT_Sprite;
		drawable->drawRoutine = drawRoutine;
		drawable->x = x;
		drawable->y = y;
		drawable->halfSize = halfSize;
		drawable->inverseCameraDistance = inverseCameraDistance;
		drawable->invert = invert;
	}
}

inline void Renderer::RenderQueuedDrawables(backbuffer_t backBuffer)
{
	for(uint8_t n = 0; n < numQueuedDrawables; n++)
	{
		QueuedDrawable& drawable = queuedDrawables[n];
		
		if(drawable.type == DT_Sprite)
		{
			int outX = drawable.x;
			uint8_t size = drawable.halfSize << 1;
			for (uint8_t x = 0; x < size; x++)
			{
				if (outX >= 0 && outX < DISPLAY_WIDTH && wBuffer[outX] < drawable.inverseCameraDistance)
				{
#if USE_GRAPHICS_MODE
					drawable.drawRoutine(backBuffer + outX, x, drawable.halfSize);
#else
					drawable.drawRoutine(backBuffer + outX * 2 + 1, x, drawable.halfSize);
#endif
				}
				outX++;
			}
			//DrawScaled(drawable.drawRoutine, drawable.x, drawable.y, drawable.halfSize, drawable.inverseCameraDistance, drawable.invert);
		}
		else
		{
			drawable.particleSystem->Draw(drawable.x, drawable.inverseCameraDistance);
		}
	}
}

int8_t Renderer::GetHorizon(int16_t x)
{
	if (x < 0)
		x = 0;
	if (x >= DISPLAY_WIDTH)
		x = DISPLAY_WIDTH - 1;
	return horizonBuffer[x];
}

bool Renderer::TransformAndCull(int16_t worldX, int16_t worldY, int16_t& outScreenX, int16_t& outScreenW)
{
	int16_t relX, relZ;
	TransformToViewSpace(worldX, worldY, relX, relZ);

	// Frustum cull
	if (relZ < CLIP_PLANE)
		return false;

	if (relX < 0 && -2 * relZ > relX)
		return false;
	if (relX > 0 && 2 * relZ < relX)
		return false;

	TransformToScreenSpace(relX, relZ, outScreenX, outScreenW);
	
	return true;
}

void Renderer::DrawObject(drawRoutine_t drawRoutine, int16_t x, int16_t y, uint8_t scale, bool invert)
{
	int16_t screenX, screenW;

	if(TransformAndCull(x, y, screenX, screenW))
	{
		// Bit of a hack: nudge sorting closer to the camera
		uint8_t inverseCameraDistance = (uint8_t)(screenW + 1);
		int16_t spriteSize = (screenW * scale) / 128;
		int8_t outY = GetHorizon(screenX);

		/*switch (anchor)
		{
		case AnchorType::Floor:
			outY += screenW - 2 * spriteSize;
			break;
		case AnchorType::Center:
			outY -= spriteSize;
			break;
		case AnchorType::BelowCenter:
			break;
		case AnchorType::Ceiling:
			outY -= screenW;
			break;
		}*/
		
		QueueSprite(drawRoutine, screenX - spriteSize, outY, (uint8_t)spriteSize, inverseCameraDistance, invert);
	}
}

inline void Renderer::DrawWeapon(backbuffer_t backBuffer)
{
	int x = DISPLAY_WIDTH / 2 + 11 + camera.tilt / 4;
	int y = camera.bob;
	uint8_t reloadTime = Game::player.reloadTime;
	
	if(reloadTime > 0)
	{
		//Platform::DrawSprite(x - reloadTime / 3 - 1, y - reloadTime / 3 - 1, handSpriteData2, 0);
		//x -= reloadTime / 3;
		//y += reloadTime / 3 + 3;
		y++;
		DrawHand2(backBuffer, x, y);
	}
	else
	{ 
		//Platform::DrawSprite(x + 2, y + 2, handSpriteData1, 0);
		DrawHand1(backBuffer, x + 2, y + 2);
	}
}

void Renderer::DrawBar(uint8_t* screenPtr, const uint8_t* iconData, uint8_t amount, uint8_t max)
{
#if 0
	constexpr uint8_t iconWidth = 8;
	constexpr uint8_t barWidth = 32;
	constexpr uint8_t unfilledBar = 0xfe;
	constexpr uint8_t filledBar = 0xc6;
	
	uint8_t fillAmount = (amount * barWidth) / max;
	uint8_t x = 0;

	while (x < iconWidth)
	{
		screenPtr[x] = pgm_read_byte(&iconData[x]);
		x++;
	}

	while (fillAmount--)
	{
		screenPtr[x++] = filledBar;
	}

	while (x < barWidth + iconWidth)
	{
		screenPtr[x++] = unfilledBar;
	}

	screenPtr[x++] = unfilledBar;
	screenPtr[x] = 0;
#endif
}

void Renderer::DrawDamageIndicator()
{
#if 0 
	uint8_t* upper = Platform::GetScreenBuffer();
	uint8_t* lower = upper + DISPLAY_WIDTH * 7;

	for (int x = 1; x < DISPLAY_WIDTH - 1; x++)
	{
		upper[x] &= 0xfe;
		lower[x] &= 0x7f;
	}

	uint8_t* ptr = Platform::GetScreenBuffer();
	for (int y = 0; y < DISPLAY_HEIGHT / 8; y++)
	{
		*ptr = 0;
		ptr += (DISPLAY_WIDTH - 1);
		*ptr = 0;
		ptr++;
	}
#endif
}


void Renderer::DrawHUD()
{
#if 0 
	constexpr uint8_t barWidth = 40;
	uint8_t* screenBuffer = Platform::GetScreenBuffer();
	uint8_t* screenPtr = screenBuffer + DISPLAY_WIDTH * 6;

	DrawBar(screenBuffer + DISPLAY_WIDTH * 7, heartSpriteData, Game::player.hp, Game::player.PLAYER_MAX_HP);
	DrawBar(screenBuffer + DISPLAY_WIDTH * 6, manaSpriteData, Game::player.mana, Game::player.PLAYER_MAX_MANA);

	if(Game::player.damageTime > 0)
		DrawDamageIndicator();

	if (Game::displayMessage)
		Font::PrintString(Game::displayMessage, 0, 0);
#endif
}

inline void Renderer::DrawWallVS(RoomDrawContext& context, int16_t viewX1, int16_t viewZ1, int16_t viewX2, int16_t viewZ2, uint8_t colour, uint8_t length)
{
	PROFILE_SECTION(PlotWall);
	wallDrawCounter++;

	int16_t vx1, vx2;
	int16_t sx1, sx2;
	int16_t w1, w2;
	bool leftEdgeVisible = true;

	uint8_t steps = length * 2;
	int16_t detailX = viewX1;
	int16_t detailZ = viewZ1;
	int16_t detailDeltaX = (viewX2 - viewX1);
	int16_t detailDeltaZ = (viewZ2 - viewZ1);

	// clip to the front pane
	if ((viewZ1 < CLIP_PLANE) && (viewZ2 < CLIP_PLANE))
		return;

	if (viewZ1 < CLIP_PLANE)
	{
		leftEdgeVisible = false;

#if USE_LOW_PRECISION_RENDERING
		viewX1 += (CLIP_PLANE - viewZ1) * (viewX2 - viewX1) / (viewZ2 - viewZ1);
#else
		viewX1 += (int32_t)(CLIP_PLANE - viewZ1) * (int32_t)(viewX2 - viewX1) / (int32_t)(viewZ2 - viewZ1);
#endif

		viewZ1 = CLIP_PLANE;
	}
	else if (viewZ2 < CLIP_PLANE)
	{
#if USE_LOW_PRECISION_RENDERING
		viewX2 += (CLIP_PLANE - viewZ2) * (viewX1 - viewX2) / (viewZ1 - viewZ2);
#else
		viewX2 += (int32_t)(CLIP_PLANE - viewZ2) * (int32_t)(viewX1 - viewX2) / (int32_t)(viewZ1 - viewZ2);
#endif
		viewZ2 = CLIP_PLANE;
	}

	// apply perspective projection
#if USE_LOW_PRECISION_RENDERING
	vx1 = (viewX1 * NEAR_PLANE * CAMERA_SCALE / viewZ1);
	vx2 = (viewX2 * NEAR_PLANE * CAMERA_SCALE / viewZ2);
#else
	vx1 = (int16_t)((int32_t)viewX1 * NEAR_PLANE * CAMERA_SCALE / viewZ1);
	vx2 = (int16_t)((int32_t)viewX2 * NEAR_PLANE * CAMERA_SCALE / viewZ2);
#endif

	// transform the end points into screen space
	sx1 = (int16_t)((DISPLAY_WIDTH / 2) + vx1);
	sx2 = (int16_t)((DISPLAY_WIDTH / 2) + vx2) - 1;

	if (sx1 >= sx2 || sx2 <= context.clipLeft || sx1 >= context.clipRight)
		return;

#if USE_LOW_PRECISION_RENDERING
	w1 = (int16_t)((CELL_SIZE / (2 << RENDER_PRECISION_SHIFT) * NEAR_PLANE * CAMERA_SCALE) / viewZ1);
	w2 = (int16_t)((CELL_SIZE / (2 << RENDER_PRECISION_SHIFT) * NEAR_PLANE * CAMERA_SCALE) / viewZ2);
#else
	w1 = (int16_t)((CELL_SIZE / 2 * NEAR_PLANE * CAMERA_SCALE) / viewZ1);
	w2 = (int16_t)((CELL_SIZE / 2 * NEAR_PLANE * CAMERA_SCALE) / viewZ2);
#endif

	currentWallId++;
	DrawWallSegment(context, sx1, w1, sx2, w2, colour);

	if (leftEdgeVisible && sx1 >= context.clipLeft && wallId[sx1] == currentWallId)
	{
		wallColourUpper[sx1] = DETAIL_COLOUR;
	}

	detailDeltaX /= steps;
	detailDeltaZ /= steps;

	for (int n = 1; n < steps; n++)
	{
		detailX += detailDeltaX;
		detailZ += detailDeltaZ;

		if (detailZ > CLIP_PLANE)
		{
			// apply perspective projection
			int16_t dvx;

#if USE_LOW_PRECISION_RENDERING
			dvx = (detailX * NEAR_PLANE * CAMERA_SCALE / detailZ);
#else
			dvx = (int16_t)((int32_t)detailX * NEAR_PLANE * CAMERA_SCALE / detailZ);
#endif

			// transform the end points into screen space
			int16_t dsx = (int16_t)((DISPLAY_WIDTH / 2) + dvx);

			if (dsx > context.clipRight)
				break;

			if (dsx >= context.clipLeft && wallId[dsx] == currentWallId)
			{
				if (n & 1)
				{
					wallColourLower[dsx] = DETAIL_COLOUR;
				}
				else
				{
					wallColourUpper[dsx] = DETAIL_COLOUR;
				}
			}
		}
	}
}

#if WITH_DOORS
void Renderer::DrawDoorVS(RoomDrawContext& context, int16_t viewX1, int16_t viewZ1, int16_t viewX2, int16_t viewZ2)
{
	wallDrawCounter++;

	int16_t vx1, vx2;
	int16_t vxMid;
	int16_t sx1, sx2;
	int16_t w1, w2;

	// clip to the front pane
	if ((viewZ1 < CLIP_PLANE) && (viewZ2 < CLIP_PLANE))
		return;

	if (viewZ1 < CLIP_PLANE)
	{
		viewX1 += (int32_t)(CLIP_PLANE - viewZ1) * (int32_t)(viewX2 - viewX1) / (int32_t)(viewZ2 - viewZ1);
		viewZ1 = CLIP_PLANE;
	}
	else if (viewZ2 < CLIP_PLANE)
	{
		viewX2 += (int32_t)(CLIP_PLANE - viewZ2) * (int32_t)(viewX1 - viewX2) / (int32_t)(viewZ1 - viewZ2);
		viewZ2 = CLIP_PLANE;
	}

	// apply perspective projection
	vx1 = (int16_t)((int32_t)viewX1 * NEAR_PLANE * CAMERA_SCALE / viewZ1);
	vx2 = (int16_t)((int32_t)viewX2 * NEAR_PLANE * CAMERA_SCALE / viewZ2);

	// transform the end points into screen space
	sx1 = (int16_t)((DISPLAY_WIDTH / 2) + vx1);
	sx2 = (int16_t)((DISPLAY_WIDTH / 2) + vx2) - 1;

	int16_t edgeX = sx2;
	int16_t handleX = sx2 - 2;

	if (sx1 > sx2)
	{
		int16_t temp = sx2;
		sx2 = sx1;
		sx1 = temp;
		temp = viewZ1;
		viewZ1 = viewZ2;
		viewZ2 = temp;
		handleX = sx1 + 2;
		edgeX = sx1;
	}

	if (sx2 <= context.clipLeft || sx1 >= context.clipRight)
		return;

	w1 = (int16_t)((CELL_SIZE / 2 * NEAR_PLANE * CAMERA_SCALE) / viewZ1);
	w2 = (int16_t)((CELL_SIZE / 2 * NEAR_PLANE * CAMERA_SCALE) / viewZ2);

	currentWallId++;
	const uint8_t doorColour = 6;
	DrawWallSegment(context, sx1, w1, sx2, w2, doorColour);

	if (edgeX >= context.clipLeft && edgeX < context.clipRight && wallId[edgeX] == currentWallId)
	{
		wallColourLower[edgeX] = 0;
		wallColourUpper[edgeX] = 0;
	}
	if (handleX >= context.clipLeft && handleX < context.clipRight && wallId[handleX] == currentWallId)
	{
		wallColourLower[handleX] = 7;
	}
}
#endif

#define PORTAL_CLIP_PLANE CLIP_PLANE
#define RASTERISE_PORTAL 0

inline bool Renderer::IsPortalVisible(RoomDrawContext& context, int16_t viewX1, int16_t viewZ1, int16_t viewX2, int16_t viewZ2, uint8_t& clipLeft, uint8_t& clipRight)
{
	int16_t vx1, vx2;
	int16_t sx1, sx2;

	// check if behind view
	if ((viewZ1 < 0) && (viewZ2 < 0))
	{
		return false;
	}
	
	if (viewZ1 <= PORTAL_CLIP_PLANE)
	{
		sx1 = context.clipLeft;
	}
	else
	{
#if USE_LOW_PRECISION_RENDERING
		vx1 = viewX1 * NEAR_PLANE * CAMERA_SCALE / viewZ1;
#else
		vx1 = (int16_t)((int32_t)viewX1 * NEAR_PLANE * CAMERA_SCALE / viewZ1);
#endif

		// transform the end points into screen space
		sx1 = (int16_t)((DISPLAY_WIDTH / 2) + vx1) - 1;
	}

	if (viewZ2 <= PORTAL_CLIP_PLANE)
	{
		sx2 = context.clipRight;
	}
	else
	{
#if USE_LOW_PRECISION_RENDERING
		vx2 = viewX2 * NEAR_PLANE * CAMERA_SCALE / viewZ2;
#else
		vx2 = (int16_t)((int32_t)viewX2 * NEAR_PLANE * CAMERA_SCALE / viewZ2);
#endif

		// transform the end points into screen space
		sx2 = (int16_t)((DISPLAY_WIDTH / 2) + vx2) + 1;
	}

	if (sx2 <= sx1 || sx2 < context.clipLeft || sx1 > context.clipRight)
		return false;
	

#if 0
	int16_t vx1, vx2;
	int16_t sx1, sx2;

	// clip to the front plane
	if ((viewZ1 < PORTAL_CLIP_PLANE) && (viewZ2 < PORTAL_CLIP_PLANE))
	{
		if (viewZ1 >= 0 || viewZ2 >= 0)
		{
			clipLeft = context.clipLeft;
			clipRight = context.clipRight;
			return true;
		}

		return false;
	}

	if (viewZ1 < PORTAL_CLIP_PLANE)
	{
		viewX1 += (int32_t)(PORTAL_CLIP_PLANE - viewZ1) * (int32_t)(viewX2 - viewX1) / (int32_t)(viewZ2 - viewZ1);
		viewZ1 = PORTAL_CLIP_PLANE;
	}
	else if (viewZ2 < PORTAL_CLIP_PLANE)
	{
		viewX2 += (int32_t)(PORTAL_CLIP_PLANE - viewZ2) * (int32_t)(viewX1 - viewX2) / (int32_t)(viewZ1 - viewZ2);
		viewZ2 = PORTAL_CLIP_PLANE;
	}

	// apply perspective projection
	vx1 = (int16_t)((int32_t)viewX1 * NEAR_PLANE * CAMERA_SCALE / viewZ1);
	vx2 = (int16_t)((int32_t)viewX2 * NEAR_PLANE * CAMERA_SCALE / viewZ2);

	// transform the end points into screen space
	sx1 = (int16_t)((DISPLAY_WIDTH / 2) + vx1);
	sx2 = (int16_t)((DISPLAY_WIDTH / 2) + vx2);

	if (sx1 >= sx2 || sx2 <= context.clipLeft || sx1 >= context.clipRight)
		return false;
#endif

#if RASTERISE_PORTAL
	int16_t w1, w2;
	w1 = (int16_t)((CELL_SIZE / 2 * NEAR_PLANE * CAMERA_SCALE) / viewZ1);
	w2 = (int16_t)((CELL_SIZE / 2 * NEAR_PLANE * CAMERA_SCALE) / viewZ2);

	int16_t dx;
	int16_t werror;
	int16_t w;
	int16_t dw;
	int8_t wstep;
	int x;
	uint8_t y;

	if (sx1 < context.clipLeft)
	{
		w1 += ((int32_t)(context.clipLeft - sx1) * (int32_t)(w2 - w1)) / (sx2 - sx1);
		sx1 = context.clipLeft;
	}

	dx = sx2 - sx1;
	werror = dx / 2;
	w = w1;

	if (w1 < w2)
	{
		dw = w2 - w1;
		wstep = 1;
	}
	else
	{
		dw = w1 - w2;
		wstep = -1;
	}

	bool visible = false;

	for (x = sx1; x < context.clipRight; x++)
	{
		if (wBuffer[x] < w)
		{
			if (!visible)
			{
				clipLeft = x;
				visible = true;
			}
			clipRight = x + 1;
		}
		else if (visible)
		{
			return true;
		}

		if (x == sx2)
		{
			break;
		}

		werror -= dw;

		while (werror < 0)
		{
			w += wstep;
			werror += dx;
		}
	}

	return visible;
#else
	clipLeft = sx1 < context.clipLeft ? context.clipLeft : sx1;
	clipRight = sx2 > context.clipRight ? context.clipRight : sx2;
	return true;
#endif
}


void Renderer::DrawGeometry(backbuffer_t backBuffer)
{
	Vertex viewSpaceVertices[MAX_ROOM_VERTICES];
	RoomDrawContext roomsToDraw[MAX_ROOMS];
	int numRoomsToDraw = 1;
	uint8_t cameraRoomIndex = Map::GetRoomIndex(camera.cellX, camera.cellY);
	const uint8_t maxPortalDepth = 2;

	memset(visibleRooms, 0, sizeof(bool) * MAX_ROOMS);

	roomsToDraw[0].Set(cameraRoomIndex, 0, DISPLAY_WIDTH, 0);
	visibleRooms[cameraRoomIndex] = true;

	for(int r = 0; r < numRoomsToDraw; r++)
	{
		RoomDrawContext& context = roomsToDraw[r];
		Room& room = Map::GetRoom(context.roomIndex);

		{
			PROFILE_SECTION(TransformVerts);

			for (int n = 0; n < room.numVertices; n++)
			{
				TransformToViewSpace(room.vertices[n].x, room.vertices[n].y, viewSpaceVertices[n].x, viewSpaceVertices[n].y);
			}
		}

		for (int n = 0; n < room.numWalls; n++)
		{
			WallSegment& wall = room.walls[n];

			if (wall.connectedRoomIndex != 0)
			{
				PROFILE_SECTION(CalcPortal);

				if (context.depth >= maxPortalDepth)
					continue;
#if WITH_DOORS
				if (room.door.isActive && !room.door.doorOpen)
					continue;
#endif

				uint8_t portalClipLeft, portalClipRight;
				if (IsPortalVisible(context, 
					viewSpaceVertices[wall.vertexA].x, viewSpaceVertices[wall.vertexA].y,
					viewSpaceVertices[wall.vertexB].x, viewSpaceVertices[wall.vertexB].y,
					portalClipLeft, portalClipRight))
				{
					if (visibleRooms[wall.connectedRoomIndex])
					{
						for (int i = r + 1; i < numRoomsToDraw; i++)
						{
							if (roomsToDraw[i].roomIndex == wall.connectedRoomIndex)
							{
								if (portalClipLeft < roomsToDraw[i].clipLeft)
								{
									roomsToDraw[i].clipLeft = portalClipLeft;
								}
								if (portalClipRight > roomsToDraw[i].clipRight)
								{
									roomsToDraw[i].clipRight = portalClipRight;
								}
							}
						}
					}
					else
					{
						visibleRooms[wall.connectedRoomIndex] = true;
						RoomDrawContext& connectedContext = roomsToDraw[numRoomsToDraw];
						numRoomsToDraw++;
						connectedContext.Set(wall.connectedRoomIndex, portalClipLeft, portalClipRight, context.depth + 1);
					}
				}
			}
			else
			{
				DrawWallVS(context, viewSpaceVertices[wall.vertexA].x, viewSpaceVertices[wall.vertexA].y,
					viewSpaceVertices[wall.vertexB].x, viewSpaceVertices[wall.vertexB].y, wall.colour, wall.length);
			}
		}

		for (int n = 0; n < room.numTorches; n++)
		{
			DrawObject(DrawTorch1, room.torches[n].x, room.torches[n].y, 75, false);
		}

#if WITH_DOORS
		if (room.door.isActive && room.door.doorOpen < MAX_DOOR_OPEN)
		{
			Vertex hingeLeftVS, hingeRightVS;

			if (room.door.isHorizontal)
			{
				TransformToViewSpace(room.door.hingeLeft.x, room.door.hingeLeft.y, hingeLeftVS.x, hingeLeftVS.y);
				TransformToViewSpace(room.door.hingeRight.x - room.door.doorOpen, room.door.hingeRight.y, hingeRightVS.x, hingeRightVS.y);
			}
			else
			{
				TransformToViewSpace(room.door.hingeLeft.x, room.door.hingeLeft.y, hingeLeftVS.x, hingeLeftVS.y);
				TransformToViewSpace(room.door.hingeRight.x, room.door.hingeRight.y - room.door.doorOpen, hingeRightVS.x, hingeRightVS.y);
			}
			DrawDoorVS(context, hingeLeftVS.x, hingeLeftVS.y, hingeRightVS.x, hingeRightVS.y);

			/*
			Vertex hingeLeftVS, hingeRightVS;
			Vertex doorLeft, doorRight;
			Vertex doorLeftVS, doorRightVS;

			if (room.door.isHorizontal)
			{
				int16_t deltaX = CELL_SIZE / 2 - room.door.doorOpen; 
				doorLeft.x = room.door.hingeLeft.x + deltaX;
				doorLeft.y = room.door.hingeLeft.y;
				doorRight.x = room.door.hingeRight.x - deltaX;
				doorRight.y = room.door.hingeRight.y;
			}
			else
			{
				int16_t deltaY = CELL_SIZE / 2 - room.door.doorOpen;
				doorLeft.x = room.door.hingeLeft.x;
				doorLeft.y = room.door.hingeLeft.y + deltaY;
				doorRight.x = room.door.hingeRight.x;
				doorRight.y = room.door.hingeRight.y - deltaY;
			}

			TransformToViewSpace(room.door.hingeLeft.x, room.door.hingeLeft.y, hingeLeftVS.x, hingeLeftVS.y);
			TransformToViewSpace(room.door.hingeRight.x, room.door.hingeRight.y, hingeRightVS.x, hingeRightVS.y);
			TransformToViewSpace(doorLeft.x, doorLeft.y, doorLeftVS.x, doorLeftVS.y);
			TransformToViewSpace(doorRight.x, doorRight.y, doorRightVS.x, doorRightVS.y);

			DrawDoorVS(context, hingeLeftVS.x, hingeLeftVS.y, doorLeftVS.x, doorLeftVS.y);
			DrawDoorVS(context, hingeRightVS.x, hingeRightVS.y, doorRightVS.x, doorRightVS.y);
			*/
		}
#endif
	}

	//unsigned char far* backBuffer = (unsigned char far*) MK_FP(0xB800, 0); 
#if 0
	char strBuffer[80];
	sprintf(strBuffer, "Rooms: %d Walls: %d Visible walls: %d       ", numRoomsToDraw, wallDrawCounter, visibleSegmentCounter);
	Platform::DrawString(backBuffer, strBuffer, 0, 20, 0xf);
#endif
}

void ClearWBuffer(unsigned char* wBuffer);
#pragma aux ClearWBuffer = \
	"mov ax, ds" /* ES:DI = wBuffer */ \
	"mov es, ax" \
	"mov cx, 80" /* 80 bytes */ \
	"mov ax, 1" /* Set to 1 */ \
	"rep stosb" \
	modify [es cx ax] \
	parm [di]

void Renderer::Render(backbuffer_t backBuffer, Player& player)
{
	globalRenderFrame++;

	camera.x = player.x;
	camera.y = player.y;
	camera.angle = player.angle;

	wallDrawCounter = 0;
	cellDrawCounter = 0;
	wallSegmentDrawCounter = 0;
	visibleSegmentCounter = 0;

	numBufferSlicesFilled = 0;
	numQueuedDrawables = 0;

	ClearWBuffer(wBuffer);

	camera.rotCos = FixedCos(-camera.angle);
	camera.rotSin = FixedSin(-camera.angle);
	camera.clipCos = FixedCos(-camera.angle + CLIP_ANGLE);
	camera.clipSin = FixedSin(-camera.angle + CLIP_ANGLE);

#if USE_LOW_PRECISION_RENDERING
	camera.rotCos >>= RENDER_ROTATION_PRECISION_SHIFT;
	camera.rotSin >>= RENDER_ROTATION_PRECISION_SHIFT;
#endif

	DrawGeometry(backBuffer);

	{
		PROFILE_SECTION(DrawObjects);
		ProjectileManager::Draw();
		EnemyManager::Draw();
		ParticleSystemManager::Draw();
	}

	/*
	

	DrawHUD();
	*/


	{
		PROFILE_SECTION(BlitWall);
#if USE_GRAPHICS_MODE
		int bufferOffset = 0;
		for (int x = 0; x < DISPLAY_WIDTH; x++)
		{
			RenderWallSlice(backBuffer + x, wBuffer[x], wallColourUpper[x], wallColourLower[x]);
		}
#else
		int bufferOffset = 1;
		for (int x = 0; x < DISPLAY_WIDTH; x++)
		{
			RenderWallSlice(backBuffer + bufferOffset, wBuffer[x], wallColourUpper[x], wallColourLower[x]);
			bufferOffset += 2;
		}
#endif
	}

	{
		PROFILE_SECTION(BlitSprite);
		RenderQueuedDrawables(backBuffer);
	}

	DrawWeapon(backBuffer);
	//printf("Cells: %d Walls: %d Segments: %d Visible: %d\n", cellDrawCounter, wallDrawCounter, wallSegmentDrawCounter, visibleSegmentCounter);
}

bool Renderer::IsCellPotentiallyVisible(uint8_t x, uint8_t y)
{
	return visibleRooms[Map::GetRoomIndex(x, y)];
}
