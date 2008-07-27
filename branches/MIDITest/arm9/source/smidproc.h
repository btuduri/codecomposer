#ifndef smidproc_h
#define smidproc_h

#include "std.h"
#include "setarm9_reg_waitcr.h"
#include "plugin_supple.h"

// macros and variables from moonshell
#define ChannelsCount (16)
#define MaxSampleRate (32768)
#define MinFramePerSecond (120)
#define MaxSamplePerFrame (MaxSampleRate/MinFramePerSecond)
#define MaxStackCount (12)
#define ReserveCount (4)

// declaration from moonshell
bool loadSoundSource(TPluginBody *pPB);
void selSetParam(u8 *data,u32 SampleRate,u32 SampleBufCount,u32 MaxChannelCount,u32 GenVolume);
bool selStart(void);
void selFree(void);
int selGetNearClock(void);
bool selNextClock(bool ShowEventMessage,bool EnableNote,int DecClock);
void selAllSoundOff(void);
bool sel_isAllTrackEOF(void);
u32 sel_GetSamplePerClockFix16(void);
void Start_smidlibDetectTotalClock();
bool Start_InitStackBuf(void);
bool Start(int FileHandle);
void Free(void);
u32 Update(s16 *lbuf,s16 *rbuf);
s32 GetPosMax(void);
s32 GetPosOffset(void);
void SetPosOffset(s32 ofs);
u32 Update(s16 *lbuf,s16 *rbuf);
bool strpcmUpdate_mainloop(void);
int updateNoteon(s16 *lbuf, s16 *rbuf, u8 channel, u8 data1, u8 data2);
bool strpcmUpdateNoteon(u8 channel, u8 data1, u8 data2);
bool initSmidlib();
void testValue(u32 Status);
void InitInterrupts(void);

#endif