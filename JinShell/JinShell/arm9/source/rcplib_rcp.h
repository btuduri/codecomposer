
#ifndef rcplib_rcp_h
#define rcplib_rcp_h

#define RCPTrackMax (18)

typedef struct {
  char Title[64+1];
  char Memo[336+1];
  u32 TimeRes;
  u32 Tempo;
  s32 PlayBias;
  u32 TrackCount;
} TRCP_Chank;

enum ERCPTrackPlayMode {ERCPTPM_Play=0,ERCPTPM_Mute=1,ERCPTPM_Mix=2,ERCPTPM_Rec=4};

#define RCP_TrackHeaderSize (44)

#define RCP_Track_LoopMax (32)

typedef struct {
  u8 *pReturn;
  u32 LoopCount;
} TRCP_Track_Loop;

enum ERCPExcCmd {EEC_None=0x00,EEC_CHExclusive=0x98,EEC_Exec=0x99,EEC_Commment=0xf6};

typedef struct {
  ERCPExcCmd cmd;
  u8 GT,Vel;
  u32 Count;
  u8 Buf[128];
} TRCP_Track_Exc;

typedef struct {
  bool EndFlag;
  u32 unuseTrackLen;
  u32 unuseTrackNum;
  bool RythmMode;
  u32 MIDICh;
  s32 KeyBias;
  s32 unuseStBias;
  ERCPTrackPlayMode PlayMode;
//  char Comment[36+1];
  u8 *DataTop,*Data,*DataEnd;
  int WaitClock;
  u8 *pReturnSameMeasure;
  u32 LoopFreeIndex;
  TRCP_Track_Loop *pLoop;
  TRCP_Track_Exc *pExc;
} TRCP_Track;

typedef struct {
  u8 *File;
  u32 FilePos;
  
  bool FastNoteOn;
  
  u32 TempoFactor;
  
  u32 SampleRate;
  u32 SamplePerClockFix16;
  
  TRCP_Track RCP_Track[RCPTrackMax];
} TRCP;

extern TRCP RCP;
extern TRCP_Chank RCP_Chank;

extern void RCP_Init(void);
extern void RCP_Free(void);

extern void RCP_ProcMetaEvent(void);
extern bool RCP_isAllTrackEOF(void);
extern void RCP_LoadRCP(u8 *FilePtr,u32 SampleRate);
extern int RCP_GetDeltaTime(TRCP_Track *pRCP_Track);
extern void RCP_ProcRCP(bool ShowMessage,bool EnableNote,TRCP_Track *pRCP_Track);

extern u32 RCP_GetSamplePerClockFix16(void);

#endif

