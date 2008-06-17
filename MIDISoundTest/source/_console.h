#ifndef _console_h
#define _console_h

#include <nds.h>
#define ShowDebugMsg
#ifdef ShowDebugMsg

// request GBA cart owner for ARM7

extern void _consolePrint(const char* pstr);
extern void _consolePrintf(const char* format, ...);
//extern void PrfStart(void);
//extern u32 PrfEnd(int data);

#else

#define _consolePrint(x)
static void _consolePrintf(const char* format, ...){};
#define PrfStart()
static u32 PrfEnd(int data){return(0);};

#endif

#endif
