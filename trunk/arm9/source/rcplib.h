
#ifndef rcplib_h
#define rcplib_h

#include "smidlib_pch.h"
#include "smidlib_mtrk.h"
#include "rcplib_rcp.h"

extern void rcplibSetParam(u8 *data,u32 SampleRate,u32 SampleBufCount,u32 MaxChannelCount,u32 GenVolume);
extern bool rcplibStart(void);
extern void rcplibFree(void);
extern int rcplibGetNearClock(void);
extern bool rcplibNextClock(bool ShowEventMessage,bool EnableNote,int DecClock);
extern void rcplibAllSoundOff(void);

#endif

