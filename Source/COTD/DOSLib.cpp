#include <dos.h>
#include <i86.h>
#include <conio.h>
#include <memory.h>
#include "DOSLib.h"

INTFUNCPTR DOSLib::oldTimerInterrupt; 
volatile long int DOSLib::milliseconds; 
INTFUNCPTR DOSLib::oldKeyboardInterrupt; 
unsigned char DOSLib::normalKeys[0x60];
unsigned char DOSLib::extendedKeys[0x60];

static unsigned char far* VRAM1 = (unsigned char far*) MK_FP(0xB800, 0);
static unsigned char far* VRAM2 = (unsigned char far*) MK_FP(0xB900, 0);
static unsigned char far* DOSLib::backBuffer = (unsigned char far*) MK_FP(0xB900, 0);

void DOSLib::Init()
{
	SetScreenMode(3, true);
	ClearScreen();
	InstallTimer();
	InstallKeyboard();
}

void DOSLib::Shutdown()
{
	ShutdownKeyboard();
	ShutdownTimer();
	SetScreenMode(3, false);
}

void DOSLib::ClearScreen()
{
	unsigned short far* VRAM = (unsigned short far*)MK_FP(0xB800, 0);
	unsigned short clear = 0x00df;
	int n;

	for (n = 0; n < 0x2000; n++)
	{
		VRAM[n] = clear;
	}
}

void DOSLib::SetScreenMode(int screenMode, bool disableBlinking)
{
	union REGS inreg, outreg;
	inreg.h.ah = 0;
	inreg.h.al = (unsigned char)screenMode;

	int86(0x10, &inreg, &outreg);

	if (disableBlinking)
	{
		// For CGA cards
		outp(0x3D8, 9);

		// For EGA and above
		inreg.h.ah = 0x10;
		inreg.h.al = 0x3;
		inreg.h.bl = 0x0;
		int86(0x10, &inreg, &outreg);
	}
}

void DOSLib::DisplayFlip()
{
	if (backBuffer == VRAM1)
	{
		backBuffer = VRAM2;
		outp(0x3d4, 0xc);
		outp(0x3d5, 0x0);
		outp(0x3d4, 0xd);
		outp(0x3d5, 0x0);
	}
	else
	{
		backBuffer = VRAM1;
		outp(0x3d4, 0xc);
		outp(0x3d5, 0x8);
		outp(0x3d4, 0xd);
		outp(0x3d5, 0x0);
	}
}

void DOSLib::InstallTimer()
{
	union REGS r;
	struct SREGS s;
	_disable();
	segread(&s);
	/* Save old interrupt vector: */
	r.h.al = 0x08;
	r.h.ah = 0x35;
	int86x(0x21, &r, &r, &s);
	oldTimerInterrupt = (INTFUNCPTR)MK_FP(s.es, r.x.bx);
	/* Install new interrupt handler: */
	milliseconds = 0;
	r.h.al = 0x08;
	r.h.ah = 0x25;
	s.ds = FP_SEG(DOSLib::TimerHandler);
	r.x.dx = FP_OFF(DOSLib::TimerHandler);
	int86x(0x21, &r, &r, &s);
	/* Set resolution of timer chip to 1ms: */
	outp(0x43, 0x36);
	outp(0x40, (unsigned char)(1103 & 0xff));
	outp(0x40, (unsigned char)((1103 >> 8) & 0xff));
	_enable();
}

void DOSLib::ShutdownTimer()
{
	union REGS r;
	struct SREGS s;
	_disable();
	segread(&s);
	/* Re-install original interrupt handler: */
	r.h.al = 0x08;
	r.h.ah = 0x25;
	s.ds = FP_SEG(oldTimerInterrupt);
	r.x.dx = FP_OFF(oldTimerInterrupt);
	int86x(0x21, &r, &r, &s);
	/* Reset timer chip resolution to 18.2...ms: */
	outp(0x43, 0x36);
	outp(0x40, 0x00);
	outp(0x40, 0x00);
	_enable();
}

void __interrupt __far DOSLib::TimerHandler(void)
{
	static unsigned long count = 0; // To keep track of original timer ticks
	++milliseconds;
	count += 1103;

	if (count >= 65536) // It is now time to call the original handler
	{
		count -= 65536;
		_chain_intr(oldTimerInterrupt);
	}
	else
		outp(0x20, 0x20); // Acknowledge interrupt
}


void DOSLib::InstallKeyboard()
{
	oldKeyboardInterrupt = _dos_getvect(0x09);
	_dos_setvect(0x09, KeyboardHandler);
}

void DOSLib::ShutdownKeyboard()
{
	if (oldKeyboardInterrupt != NULL)
	{
		_dos_setvect(0x09, oldKeyboardInterrupt);
		oldKeyboardInterrupt = NULL;
	}
}

void __interrupt __far DOSLib::KeyboardHandler(void)
{
	static unsigned char buffer;
	unsigned char rawcode;
	unsigned char make_break;
	int scancode;
	unsigned char temp;

	rawcode = inp(0x60); /* read scancode from keyboard controller */

	// Tell the XT keyboard controller to clear the key
	outp(0x61, (temp = inp(0x61)) | 0x80);
	outp(0x61, temp);

	make_break = !(rawcode & 0x80); /* bit 7: 0 = make, 1 = break */
	scancode = rawcode & 0x7F;

	if (buffer == 0xE0) { /* second byte of an extended key */
		if (scancode < 0x60) {
			extendedKeys[scancode] = make_break;
		}
		buffer = 0;
	}
	else if (buffer >= 0xE1 && buffer <= 0xE2) {
		buffer = 0; /* ingore these extended keys */
	}
	else if (rawcode >= 0xE0 && rawcode <= 0xE2) {
		buffer = rawcode; /* first byte of an extended key */
	}
	else if (scancode < 0x60) {
		normalKeys[scancode] = make_break;
	}

	outp(0x20, 0x20); /* must send EOI to finish interrupt */
}
