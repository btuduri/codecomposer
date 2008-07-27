#ifndef smidlib_h
#define smidlib_h

#include <nds/jtypes.h>

void smidlibSetParam(u8 *data,u32 SampleRate,u32 SampleBufCount,u32 MaxChannelCount,u32 GenVolume);
void smidlibSetSoundParam(u32 SampleRate,u32 SampleBufCount,u32 MaxChannelCount,u32 GenVolume);
bool smidlibStart(void);
void smidlibFree(void);
int smidlibGetNearClock(void);
bool smidlibNextClock(bool ShowEventMessage,bool EnableNote,int DecClock);
void smidlibAllSoundOff(void);

#endif