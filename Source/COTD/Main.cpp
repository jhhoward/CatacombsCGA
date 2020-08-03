#include <stdio.h>
#include <stdarg.h>
#include "Platform.h"
#include "DOSLib.h"
#include "Game.h"

static uint8_t inputState = 0;

uint8_t Platform::GetInput(void)
{
	return inputState;
}

void Platform::PlaySound(const uint16_t* audioPattern)
{

}

bool Platform::IsAudioEnabled()
{
	return false;
}

void Platform::SetAudioEnabled(bool isEnabled)
{

}

void Platform::ExpectLoadDelay()
{

}

void Platform::FillScreen(uint8_t col)
{

}

void Platform::PutPixel(uint8_t x, uint8_t y, uint8_t colour)
{

}

void Platform::DrawBitmap(int16_t x, int16_t y, const uint8_t *bitmap)
{

}

void Platform::DrawSolidBitmap(int16_t x, int16_t y, const uint8_t *bitmap)
{

}

void Platform::DrawSprite(int16_t x, int16_t y, const uint8_t *bitmap, const uint8_t *mask, uint8_t frame, uint8_t mask_frame)
{

}

void Platform::DrawSprite(int16_t x, int16_t y, const uint8_t *bitmap, uint8_t frame)
{

}

void Platform::DrawString(unsigned char far* vram, const char* string, int x, int y, unsigned char colourAttribute)
{
	vram += y * 160 + x * 2;
	while (*string)
	{
		*vram++ = *string++;
		*vram++ = colourAttribute;
	}
}

void Platform::Log(const char* format, ...)
{
	static int logY = 20;
	//unsigned char far* backBuffer = (unsigned char far*) MK_FP(0xB800, 0);
	char strBuffer[80];
	va_list argptr;
	va_start(argptr, format);
	vsprintf(strBuffer, format, argptr);
	va_end(argptr);

	DrawString(DOSLib::backBuffer, strBuffer, 0, logY, 0xf);
	DrawString(DOSLib::frontBuffer, strBuffer, 0, logY, 0xf);
	logY++;
	if (logY >= 25)
	{
		logY = 20;
	}
}

#include <stdlib.h>
#define FIRE_HEIGHT 25
#define FIRE_WIDTH 80
char fireValues[FIRE_WIDTH * FIRE_HEIGHT];
// black, brown, red, yellow, white
unsigned short palette[16] =
{
	0x0000,		// black
	0x04b1,
	0x4400,		// red  
	0x46b1,
	0x6600,		// brown
	0x6eb1,
	0xee00,		// yellow
	0xefb1,
	0xff00,		// white
};

unsigned short palette2[16] =
{
	0x0000,		// black
	0x04b0,
	0x04b1,
	0x04b2,
	0x4400,		// red  
	0x46b0,
	0x46b1,
	0x46b2,
	0x6600,		// brown
	0x6eb0,
	0x6eb1,
	0x6eb2,
	0xee00,		// yellow
	0xefb0,
	0xefb2,
	0xff00,		// white
};

#define NUM_RANDOM_VALUES 2048
char fireRandValues[NUM_RANDOM_VALUES + 24];

#define FIRE_PX(x) \
	backBuffer[x] = palette[fireValues[x]];
#define FIRE_PX8(x) \
	FIRE_PX(x) \
	FIRE_PX(x + 1) \
	FIRE_PX(x + 2) \
	FIRE_PX(x + 3) \
	FIRE_PX(x + 4) \
	FIRE_PX(x + 5) \
	FIRE_PX(x + 6) \
	FIRE_PX(x + 7) 
#define FIRE_PX80(x) \
	FIRE_PX8(x) \
	FIRE_PX8(x + 8) \
	FIRE_PX8(x + 16) \
	FIRE_PX8(x + 24) \
	FIRE_PX8(x + 32) \
	FIRE_PX8(x + 40) \
	FIRE_PX8(x + 48) \
	FIRE_PX8(x + 56) \
	FIRE_PX8(x + 64) \
	FIRE_PX8(x + 72) 

#define SPREAD_FIRE() \
	random = fireRandValues[fireRandIndex++]; \
	val = fireValues[index]; \
	if (val > 0) val -= (random < 3) ? 1: 0; \
	dstIndex = index + random - 1 - FIRE_WIDTH; \
	fireValues[dstIndex] = val; \
	index += FIRE_WIDTH;
#define SPREAD_FIRE5() \
	SPREAD_FIRE() \
	SPREAD_FIRE() \
	SPREAD_FIRE() \
	SPREAD_FIRE() \
	SPREAD_FIRE()
#define SPREAD_FIRE24() \
	SPREAD_FIRE5() \
	SPREAD_FIRE5() \
	SPREAD_FIRE5() \
	SPREAD_FIRE5() \
	SPREAD_FIRE() \
	SPREAD_FIRE() \
	SPREAD_FIRE() \
	SPREAD_FIRE() 

void Fire()
{
	unsigned short far* backBuffer = (unsigned short far*) MK_FP(0xB800, 0);

	for (int x = 0; x < FIRE_WIDTH * FIRE_HEIGHT; x++)
	{
		fireValues[x] = 0;
	}
	for (int x = 0; x < FIRE_WIDTH; x++)
	{
		fireValues[(FIRE_HEIGHT - 1) * FIRE_WIDTH + x] = 8; // 0xf;
	}

	for (int n = 0; n < NUM_RANDOM_VALUES + 24; n++)
	{
		fireRandValues[n] = (rand() & 3);
	}
	int fireRandIndex = 0;

	//for (int y = 0; y < 16; y++)
	//{
	//	for (int x = 0; x < DISPLAY_WIDTH; x++)
	//	{
	//		fireValues[y * DISPLAY_WIDTH + x] = y;
	//	}
	//}

	bool endingFire = false;
	int framesLeft = 20;

	while (!endingFire || framesLeft > 0)
	{
		if (DOSLib::normalKeys[1])
		{
			if (endingFire)
			{
				//framesLeft = 0;
			}
			else
			{
				endingFire = true;
				for (int x = 0; x < FIRE_WIDTH; x++)
				{
					fireValues[(FIRE_HEIGHT - 1) * FIRE_WIDTH + x] = 0;
				}
			}
		}
		if (endingFire)
		{
			framesLeft--;
		}

		for (int x = 0; x < FIRE_WIDTH * FIRE_HEIGHT; x += 80)
		{
			//backBuffer[x] = palette[fireValues[x]];
			FIRE_PX80(x)
		}
		for (int x = 0; x < FIRE_WIDTH; x++)
		{
			int index = FIRE_WIDTH + x;
			unsigned char random;
			unsigned char val;
			int dstIndex;
			SPREAD_FIRE24()

			fireRandIndex &= (NUM_RANDOM_VALUES - 1); 

			/*
			for (int y = 1; y < FIRE_HEIGHT; y++)
			{
				unsigned char random = fireRandValues[fireRandIndex];
				fireRandIndex = (fireRandIndex + 1) & (NUM_RANDOM_VALUES - 1);
				unsigned char val = fireValues[index];
				if (val > 0)
					val -= (random & 1);
				//index += random - 1;
				fireValues[index + random - 1 - FIRE_WIDTH] = val;
				index += FIRE_WIDTH;
			}
			*/
		}
	}
}

int main()
{
	DOSLib::Init();
	Game::Init();

	int16_t tickAccum = 0;
	const int maxFrameSkip = 5;
	unsigned long lastTimingSample = DOSLib::GetTime();
	const int16_t frameDuration = 1000 / TARGET_FRAMERATE;
	unsigned long fpsTimer = DOSLib::GetTime();
	int fpsCounter = 0;
	int fps = 0;

	//Fire();

	DOSLib::ClearScreen();

	while (1)
	{
		unsigned long timingSample = DOSLib::GetTime();
		int framesSimulated = 0;
		tickAccum += (timingSample - lastTimingSample);
		lastTimingSample = timingSample;

		while (tickAccum > frameDuration)
		{
			inputState = 0;

			if (DOSLib::normalKeys[0x48] || DOSLib::extendedKeys[0x48])
				inputState |= INPUT_UP;
			if (DOSLib::normalKeys[0x4b] || DOSLib::extendedKeys[0x4b])
				inputState |= INPUT_LEFT;
			if (DOSLib::normalKeys[0x4d] || DOSLib::extendedKeys[0x4d])
				inputState |= INPUT_RIGHT;
			if (DOSLib::normalKeys[0x50] || DOSLib::extendedKeys[0x50])
				inputState |= INPUT_DOWN;
			if (DOSLib::normalKeys[0x1d] || DOSLib::extendedKeys[0x1d])		// Ctrl
				inputState |= INPUT_B;
			if (DOSLib::normalKeys[0x38] || DOSLib::extendedKeys[0x38])		// Alt
				inputState |= INPUT_A;

			Game::Tick();
			tickAccum -= frameDuration;
			framesSimulated++;

			if (framesSimulated > maxFrameSkip)
			{
				tickAccum = 0;
			}
		}

		if (framesSimulated > 0)
		{
			Game::Draw(DOSLib::backBuffer);
			DOSLib::DisplayFlip();
			fpsCounter++;
		}

		if (DOSLib::GetTime() > fpsTimer)
		{
			fpsTimer = DOSLib::GetTime() + 1000;
			fps = fpsCounter;
			fpsCounter = 0;

			static char fpsMessage[20];
			sprintf(fpsMessage, "FPS: %d    ", fps);
			Platform::DrawString(DOSLib::backBuffer, fpsMessage, 0, 24, 0x0f);
			Platform::DrawString(DOSLib::frontBuffer, fpsMessage, 0, 24, 0x0f);
		}

		if (DOSLib::normalKeys[1])	// Escape
		{
			break;
		}
	}

	DOSLib::Shutdown();

	return 0;
}