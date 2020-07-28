#pragma once
#include <dos.h>
#include <i86.h>
#include <conio.h>

typedef void (__interrupt __far* INTFUNCPTR)(void);

class DOSLib
{
public:
	static void Init();
	static void Shutdown();

	static long int GetTime() { return milliseconds; }

	static void DisplayFlip();

	static unsigned char normalKeys[0x60];
	static unsigned char extendedKeys[0x60];

	static unsigned char far* backBuffer;

private:
	// Video
	static void SetScreenMode(int screenMode, bool disableBlinking);
	static void ClearScreen();

	// Timer
	static void InstallTimer();
	static void ShutdownTimer();
	static void __interrupt __far TimerHandler(void);

	static INTFUNCPTR oldTimerInterrupt; // Original interrupt handler
	static volatile long int milliseconds; // Elapsed time in milliseconds

	// Keyboard
	static void InstallKeyboard();
	static void ShutdownKeyboard();
	static void __interrupt __far KeyboardHandler(void);

	static INTFUNCPTR oldKeyboardInterrupt; // Original keyboard handler

};


