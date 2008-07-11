
#ifndef smidlib_pch_h
#define smidlib_pch_h

#define PCHCountMax (40)
#define ChannelsCount (16)
#define ProgramPatchHeaderSize (sizeof(TProgram)-4)
#define PCMStackCount (256)
#define ChannelInfoCount ChannelsCount

enum EEnvState {EES_None=0,EES_Attack=1,EES_Decay=2,EES_Sustain=3,EES_Release3=4,EES_Release4=5,EES_Release5=6};

typedef struct {
  u32 SampleRate;
  u32 RootFreq; // fix16.16
  u32 MasterVolume;
  u32 FractionStart;
  u32 FractionEnd;
  u32 Length,LoopStart,LoopEnd;
  u32 LoopFlag,s16Flag;
  u32 MasterPanpot;
  u32 Note;
  u32 VibSweep;
  u32 VibRatio;
  int VibDepth;
  bool EnvelopeFlag;
  int EnvRate[6];
  u32 EnvOfs[6];
  u32 PCMOffset;
  void *pData;
} TProgram;

typedef struct {
  u32 PatchCount;
  TProgram *pPrg;
} TProgramPatch;

typedef struct {
  int FileHandle;
  u32 FileOffset;
  
  u32 DataOffset[128];
  TProgramPatch *ppData[128];
} TProgramMap;

typedef struct {
  TProgramMap *pPrgMaps[128];
} TVariationMap;

typedef struct {
  u32 PCMFileOffset;
  void *pData;
} TPCMStack;

typedef struct {
  bool DrumMode;
  u32 Vol,Exp,Reverb;
  
  bool Pedal;
} TChannelInfo;

typedef struct {
  bool Enabled;
  u32 ChannelNum;
  TChannelInfo *pChannelInfo;
  
  u32 OnClock,OffClock;
  
  u32 GT;
  
  bool VibEnable;
  u32 VibSweepCur;
  u32 VibCur;
  int VibPhase;
  
  bool ModEnable;
  u32 ModCur;
  int ModPhase;
  
  enum EEnvState EnvState;
  int EnvSpeed;
  u32 EnvCurLevel;
  u32 EnvEndLevel;
  u32 EnvRelLevel;
  
  TProgram *pPrg;
} TPCH1;

typedef struct {
  u32 Note,Vel;
  
  u32 Panpot;
  
  u32 VibSweepAdd;
  u32 VibAdd;
  int VibDepth;
  
  u32 ModAdd;
  int ModDepth;
  
  u32 PrgPos;
  u32 PrgMstVol;
  u32 PrgVol;
  
  u32 FreqAddFix16;
  u32 FreqCurFix16;
  s32 LastSampleData,CurSampleData;
} TPCH2;

extern bool MemoryOverflowFlag;

extern void PCH_SetProgramMap(int FileHandle);
extern void PCH_FreeProgramMap(void);
extern bool PCH_LoadProgram(s32 Note,u32 var,u32 prg,bool DrumMode);

extern void PCH_SetProgramMap_Load(TProgramMap *pPrgMap, int FileHandle,u32 FileOffset);

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

