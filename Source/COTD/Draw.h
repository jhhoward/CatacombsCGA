#pragma once
#include "Defines.h"

#define PERSPECTIVE_LOOKUP_TABLE_SIZE 8192

struct Camera
{
	int16_t x, y;
	uint8_t angle;
	int16_t rotCos, rotSin;
	int16_t clipCos, clipSin;
	uint8_t cellX, cellY;
	int8_t tilt;
	int8_t bob;
	uint8_t shakeTime;
};

enum DrawableType
{
	DT_Sprite = 0,
	DT_ParticleSystem = 1
};

struct QueuedDrawable
{
	union
	{
		const uint16_t* spriteData;
		struct ParticleSystem* particleSystem;
	};
	
	DrawableType type : 1;
	bool invert : 1;
	int8_t x;
	int8_t y;
	uint8_t halfSize;
	uint8_t inverseCameraDistance;
};

struct RoomDrawContext
{
	uint8_t roomIndex;
	uint8_t clipLeft, clipRight;
	uint8_t depth;

	void Set(uint8_t inRoomIndex, uint8_t inClipLeft, uint8_t inClipRight, uint8_t inDepth)
	{
		roomIndex = inRoomIndex;
		clipLeft = inClipLeft;
		clipRight = inClipRight;
		depth = inDepth;
	}
};

class Room;

class Renderer
{
public:
	static Camera camera;
	static uint8_t wBuffer[DISPLAY_WIDTH];
	static uint8_t wallColourUpper[DISPLAY_WIDTH];
	static uint8_t wallColourLower[DISPLAY_WIDTH];
	static uint8_t globalRenderFrame;

	static void Render(backbuffer_t backBuffer, class Player& player);

	static void DrawObject(const uint16_t* spriteData, int16_t x, int16_t y, uint8_t scale = 128, bool invert = false);
	static QueuedDrawable* CreateQueuedDrawable(uint8_t inverseCameraDistance);
	static int8_t GetHorizon(int16_t x);
	
	static bool TransformAndCull(int16_t worldX, int16_t worldY, int16_t& outScreenX, int16_t& outScreenW);
	
private:
	static int8_t horizonBuffer[DISPLAY_WIDTH];
	static QueuedDrawable queuedDrawables[MAX_QUEUED_DRAWABLES];
	static uint8_t numQueuedDrawables;
	static uint8_t numBufferSlicesFilled;
	static bool visibleRooms[MAX_ROOMS];
	static uint8_t wallId[DISPLAY_WIDTH];
	static uint8_t currentWallId;

	static void DrawWallSegment(RoomDrawContext& context, int16_t x1, int16_t w1, int16_t x2, int16_t w2, uint8_t colour);
	static void DrawWallVS(RoomDrawContext& context, int16_t viewX1, int16_t viewZ1, int16_t viewX2, int16_t viewZ2, uint8_t colour, uint8_t length);

#if WITH_DOORS
	static void DrawDoorVS(RoomDrawContext& context, int16_t viewX1, int16_t viewZ1, int16_t viewX2, int16_t viewZ2);
#endif

	static bool IsPortalVisible(RoomDrawContext& context, int16_t viewX1, int16_t viewZ1, int16_t viewX2, int16_t viewZ2, uint8_t& clipLeft, uint8_t& clipRight);


	static bool isFrustrumClipped(int16_t x, int16_t y);
	static void TransformToViewSpace(int16_t x, int16_t y, int16_t& outX, int16_t& outY);
	static void TransformToScreenSpace(int16_t viewX, int16_t viewZ, int16_t& outX, int16_t& outW);
	
	static void DrawGeometry(backbuffer_t backBuffer);

	static void DrawCell(uint8_t x, uint8_t y);
	static void DrawCells();
	static void DrawWeapon(backbuffer_t backBuffer);
	static void DrawHUD();
	static void DrawBar(uint8_t* screenPtr, const uint8_t* iconData, uint8_t amount, uint8_t max);
	static void DrawDamageIndicator();
	
	static void QueueSprite(const uint16_t* data, int8_t x, int8_t y, uint8_t halfSize, uint8_t inverseCameraDistance, bool invert = false);
	static void RenderQueuedDrawables();
};
