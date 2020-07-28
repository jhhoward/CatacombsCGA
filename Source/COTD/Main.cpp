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

void DrawString(unsigned char far* vram, const char* string, int x, int y, unsigned char colourAttribute)
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
	unsigned char far* backBuffer = (unsigned char far*) MK_FP(0xB800, 0);
	char strBuffer[80];
	va_list argptr;
	va_start(argptr, format);
	vsprintf(strBuffer, format, argptr);
	va_end(argptr);

	DrawString(backBuffer, strBuffer, 0, logY, 0xf);
	logY++;
	if (logY >= 25)
	{
		logY = 20;
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

		Game::Draw();

		//DOSLib::DisplayFlip();

		fpsCounter++;
		if (DOSLib::GetTime() > fpsTimer)
		{
			fpsTimer = DOSLib::GetTime() + 1000;
			fps = fpsCounter;
			fpsCounter = 0;

			static char fpsMessage[20];
			unsigned char far* backBuffer = (unsigned char far*) MK_FP(0xB800, 0);
			sprintf(fpsMessage, "FPS: %d    ", fps);
			DrawString(backBuffer, fpsMessage, 0, 24, 0x0f);
		}

		if (DOSLib::normalKeys[1])	// Escape
		{
			break;
		}
	}

	DOSLib::Shutdown();

	return 0;
}