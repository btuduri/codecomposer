
#ifndef smidlib_pch_h
#define smidlib_pch_h

#include "nds.h"

#include <stdio.h>
#include <stdlib.h>

#define ChannelsCount (16)

extern bool MemoryOverflowFlag;

extern void PCH_SetProgramMap(FILE* FileHandle);
extern void PCH_FreeProgramMap(void);
extern bool PCH_LoadProgram(s32 Note,u32 var,u32 prg,bool DrumMode);

extern bool PCH_Init(u32 _SampleRate,u32 _SampleBufCount,u32 MaxChannelCount,u32 _GenVolume);
extern void PCH_Free(void);

extern void PCH_AllSoundOff(void);
extern void PCH_AllNoteOff(u32 trk);

extern void PCH_NextClock(void);

extern void PCH_ChangeVolume(u32 trk,u32 v);
extern void PCH_ChangeExpression(u32 trk,u32 e);
extern void PCH_ChangePitchBend(u32 trk,s32 Pitch);
extern void PCH_ChangePanpot(u32 trk,u32 p);
extern void PCH_ChangeModLevel(u32 trk,u32 ModLevel);

extern void PCH_NoteOn(u32 trk,u32 GT,s32 Note,s32 Pitch,u32 Vol,u32 Exp,u32 Vel,u32 var,u32 prg,u32 panpot,u32 reverb,bool DrumMode,u32 ModLevel);
extern void PCH_NoteOff(u32 trk,u32 Note,bool DrumMode);
extern void PCH_PedalOn(u32 trk);
extern void PCH_PedalOff(u32 trk);

extern bool PCH_RequestRender(u32 TagChannel);
extern void PCH_RenderStart(u32 SampleCount);
extern void PCH_Render(u32 TagChannel,s32 *buf,u32 SampleCount);
extern void PCH_RenderEnd(void);

extern u32 PCH_GetReverb(u32 TagChannel);

extern int PCH_GT_GetNearClock(void);
extern void PCH_GT_DecClock(u32 clk);

extern bool PCH_isDrumMap(u32 TagChannel);

extern void TTAC_Decode_8bit(u8 *pCodeBuf,s8 *buf,u32 DecompressedSamplesCount);
extern void TTAC_Decode_16bit(u8 *pCodeBuf,s16 *buf,u32 DecompressedSamplesCount);

extern "C" {
  void TTAC_Decode_8bit_asm(u8 *pCodeBuf,s8 *buf,u32 DecompressedSamplesCount);
  void TTAC_Decode_6bit_asm(u8 *pCodeBuf,s8 *buf,u32 DecompressedSamplesCount);
  void TTAC_Decode_16bit_asm(u8 *pCodeBuf,s16 *buf,u32 DecompressedSamplesCount);
  void TTAC_Decode_12bit_asm(u8 *pCodeBuf,s16 *buf,u32 DecompressedSamplesCount);
}

#endif

