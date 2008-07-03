
#ifndef _console_h
#define _console_h

#ifdef __cplusplus

extern void _consoleInitDefault(u16* map, u16* charBase, u8 bitDepth);

#ifdef ShowDebugMsg
extern void _consolePrint(const char* s);
extern void _consolePrintf(const char* format, ...);
#else
static inline void _consolePrint(const char* s)
{
}
static inline void _consolePrintf(const char* format, ...)
{
}
#endif

extern void _consolePrintChar(char c);

extern void _consoleClear(void);

#endif

#ifdef __cplusplus
extern "C" {
#endif

void _consolePrintSet(int x, int y);
int _consoleGetPrintSetY(void);

#ifdef ShowDebugMsg
void _consolePrintOne(char *str,u32 v);
#else
static inline void _consolePrintOne(char *str,u32 v)
{
}
#endif

#ifdef __cplusplus
}
#endif

#endif
