#include <stdio.h>
#include <stdarg.h>
#include "Platform.h"
#include "DOSLib.h"
#include "Game.h"
#include "Profiler.h"

static uint8_t inputState = 0;

unsigned long Profiler::timerValue[NumProfilerSectionTypes];

#if USE_GRAPHICS_MODE

unsigned char windowBackBuffer[VIEWPORT_WIDTH * VIEWPORT_HEIGHT];

void BlitWindow(unsigned char* src, unsigned int dstSeg);
#if 0
#pragma aux BlitWindow = \
	"mov ax, 0" \
	"mov di, ax" \
	"mov dx, 30"		/* 30 lines */ \
	"_blitLine:" \
	"mov cx, 40" 		/* Copy 80 bytes */ \
	"push si" \
	"rep movsw" 		/* ES:DI <- DS:SI */ \
	"pop si" \
	"mov cx, 40" 		/* Copy 80 bytes */ \
	"rep movsw" 		/* ES:DI <- DS:SI */ \
	"dec dx" \
	"jnz _blitLine" \
	modify [ax dx di cx si] \
	parm [si] [es];
#else
#pragma aux BlitWindow = \
	"mov ax, 322" \
	"mov di, ax" \
	"mov dx, 30"		/* 30 lines */ \
	"_blitLine:" \
	"mov cx, 38" 		/* Copy 76 bytes */ \
	"push si" \
	"rep movsw" 		/* ES:DI <- DS:SI */ \
	"add di, 4" \
	"pop si" \
	"mov cx, 38" 		/* Copy 76 bytes */ \
	"rep movsw" 		/* ES:DI <- DS:SI */ \
	"add di, 4" \
	"dec dx" \
	"jnz _blitLine" \
	modify [ax dx di cx si] \
	parm [si] [es];
#endif


void BlitWindowPlane1(unsigned char* src);
#pragma aux BlitWindowPlane1 = \
	"mov ax, 0xb850" 	/* ES:DI = 0xb8000 */ \
	"mov es, ax" \
	"mov ax, 0" \
	"mov di, ax" \
	"mov dx, 15"		/* 20 loops x3 lines */ \
	"_blitLine:" \
	"mov cx, 40" 		/* Copy 80 bytes */ \
	"push si" \
	"rep movsw" 		/* ES:DI <- DS:SI */ \
	"pop si" \
	"mov cx, 40" 		/* Copy 80 bytes */ \
	"rep movsw" 		/* ES:DI <- DS:SI */ \
	"mov cx, 40" 		/* Copy 80 bytes */ \
	"rep movsw" 		/* ES:DI <- DS:SI */ \
	"dec dx" \
	"jnz _blitLine" \
	modify [ax dx di cx si] \
	parm [si];

void BlitWindowPlane2(unsigned char* src);
#pragma aux BlitWindowPlane2 = \
	"mov ax, 0xba50" 	/* ES:DI = 0xba000 */ \
	"mov es, ax" \
	"mov ax, 0" \
	"mov di, ax" \
	"mov dx, 15"		/* 20 loops x3 lines */ \
	"_blitLine:" \
	"mov cx, 40" 		/* Copy 80 bytes */ \
	"rep movsw" 		/* ES:DI <- DS:SI */ \
	"mov cx, 40" 		/* Copy 80 bytes */ \
	"push si" \
	"rep movsw" 		/* ES:DI <- DS:SI */ \
	"pop si" \
	"mov cx, 40" 		/* Copy 80 bytes */ \
	"rep movsw" 		/* ES:DI <- DS:SI */ \
	"dec dx" \
	"jnz _blitLine" \
	modify [ax dx di cx si] \
	parm [si];

#endif

uint8_t Platform::GetInput(void)
{
	return inputState;
}

void Platform::PlaySound(const uint16_t* audioPattern)
{

}

long Platform::GetTime()
{
	return DOSLib::GetTime();
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
#if USE_GRAPHICS_MODE
	backbuffer_t backBuffer = windowBackBuffer;

	//int offset = (((y << 6) + (y << 4) + x));
	int offset = y * VIEWPORT_WIDTH + x;

	if (offset > (VIEWPORT_WIDTH * VIEWPORT_HEIGHT))
		return;

	backBuffer[offset] = colour;

#else
	backbuffer_t backBuffer = DOSLib::backBuffer;
	uint8_t row = y >> 1;
	//backBuffer += (row << 6) + (row << 4) + x * 2 + 1;
	int offset = (((row << 6) + (row << 4) + x) << 1) + 1;

	if (offset > (160 * 20))
		return;

	backBuffer += offset;

	if (y & 1)
	{
		*backBuffer = (*backBuffer & 0xf) | (colour << 4);
	}
	else
	{
		*backBuffer = (*backBuffer & 0xf0) | (colour);
	}
#endif
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
#if !USE_GRAPHICS_MODE
	vram += y * 160 + x * 2;
	while (*string)
	{
		*vram++ = *string++;
		*vram++ = colourAttribute;
	}
#endif
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
	bool interlace = false;
	bool narrow = false;

	//Fire();

	DOSLib::ClearScreen();

	while (1)
	{
#if WITH_PROFILER
		Profiler::ResetTimers();
#endif

		unsigned long timingSample = DOSLib::GetTime();
		int framesSimulated = 0;
		tickAccum += (timingSample - lastTimingSample);
		lastTimingSample = timingSample;

		if (DOSLib::normalKeys[0x14])
		{
			tickAccum = 0;
			framesSimulated = 1;
		}

		if (tickAccum > frameDuration)
		{
			PROFILE_SECTION(Tick);

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
		}

		if (framesSimulated > 0)
		{
#if USE_GRAPHICS_MODE
			Game::Draw(windowBackBuffer);
#else
			Game::Draw(DOSLib::backBuffer);
			DOSLib::DisplayFlip();
#endif
			fpsCounter++;
		}

#if USE_GRAPHICS_MODE
		if (DOSLib::normalKeys[0x17]) // I
		{
			interlace = !interlace;
			while (DOSLib::normalKeys[0x17]);
		}
		if (DOSLib::normalKeys[0x31]) // N
		{
			narrow = !narrow;
			while (DOSLib::normalKeys[0x31]);
		}

		if(interlace)
		{
			static bool even = true;
			even = !even;

			if (even)
				BlitWindow(windowBackBuffer, 0xb800);
			else
				BlitWindow(windowBackBuffer, 0xba00);
		}
		else
		{
			if (narrow)
			{
				BlitWindowPlane1(windowBackBuffer);
				BlitWindowPlane2(windowBackBuffer);
			}
			else
			{
				BlitWindow(windowBackBuffer, 0xb800);
				BlitWindow(windowBackBuffer, 0xba00);
			}
		}
#endif

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

#if WITH_PROFILER
		{
			static char profileStr[80];
			sprintf(profileStr, "Dr:%3ld Tk:%3ld PW:%3ld BW:%3ld Sp:%3ld Po:%3ld Ob:%3ld Tr:%3ld ",
				Profiler::timerValue[Profile_Draw],
				Profiler::timerValue[Profile_Tick],
				Profiler::timerValue[Profile_PlotWall],
				Profiler::timerValue[Profile_BlitWall],
				Profiler::timerValue[Profile_BlitSprite],
				Profiler::timerValue[Profile_CalcPortal],
				Profiler::timerValue[Profile_DrawObjects],
				Profiler::timerValue[Profile_TransformVerts]
			);
			Platform::DrawString(DOSLib::backBuffer, profileStr, 0, 23, 0x0f);
			Platform::DrawString(DOSLib::frontBuffer, profileStr, 0, 23, 0x0f);
		}
#endif

		if (DOSLib::normalKeys[1])	// Escape
		{
			break;
		}
	}

	DOSLib::Shutdown();

	printf("FPS: %d\n", fps);
	getchar();

	return 0;
}
