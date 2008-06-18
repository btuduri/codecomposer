#include <nds.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "_console.h"
#include "_const.h"
#include "std.h"
#include "inifile.h"
#include "smidlib.h"
#include "memtool.h"

#include "filesys.h"
#include "shell.h"

#include "plugin_supple.h"

#define MaxSampleRate (32768)
#define MinFramePerSecond (120)
#define MaxSamplePerFrame (MaxSampleRate/MinFramePerSecond)
#define MaxStackCount (12)
#define ReserveCount (4)

// The following two line is 
// enum EFileFormat {EFF_MID,EFF_RCP};
// static EFileFormat FileFormat;

static u8 *DeflateBuf=NULL;
static u32 DeflateSize;
static u32 SamplePerFrame;
static int TotalClock,CurrentClock;
static u32 ClockCur;
static ALIGNED_VAR_IN_DTCM s32 DTCM_tmpbuf[MaxSamplePerFrame*2];
static ALIGNED_VAR_IN_DTCM s32 DTCM_dstbuf[MaxSamplePerFrame*2];

static u32 StackIndex;
static u32 StackCount;
static s32 *pStackBuf[ChannelsCount][MaxStackCount];
static s32 StackLastCount[ChannelsCount];
static void *pReserve[ReserveCount];
static u32 SoundFontLoadTimeus;

//---------------------------------------------------------------------------------
int main(void) {
//---------------------------------------------------------------------------------
	touchPosition touchXY;

	irqInit();
	irqEnable(IRQ_VBLANK);

	videoSetMode(0);	//not using the main screen
	videoSetModeSub(MODE_0_2D | DISPLAY_BG0_ACTIVE);	//sub bg 0 will be used to print text
	vramSetBankC(VRAM_C_SUB_BG);

	SUB_BG0_CR = BG_MAP_BASE(31);

	BG_PALETTE_SUB[255] = RGB15(31,31,31);	//by default font will be rendered with color 255

	//consoleInit() is a lot more flexible but this gets you up and running quick
	consoleInitDefault((u16*)SCREEN_BASE_BLOCK_SUB(31), (u16*)CHAR_BASE_BLOCK_SUB(0), 16);

	iprintf("\n\n\tHello DS dev'rs\n");
	iprintf("\twww.drunkencoders.com");

	while(1) {

		touchXY=touchReadXY();
		iprintf("\x1b[10;0HTouch x = %04X, %04X\n", touchXY.x, touchXY.px);
		iprintf("Touch y = %04X, %04X\n", touchXY.y, touchXY.py);

		swiWaitForVBlank();
	}

	Shell_SkinOpenFile("midi.mid");

	return 0;
}

void Free(void);

static void selSetParam(u8 *data,u32 SampleRate,u32 SampleBufCount,u32 MaxChannelCount,u32 GenVolume)
{
	smidlibSetParam(DeflateBuf,SampleRate,SampleBufCount,MaxChannelCount,GenVolume);
}

static bool selStart(void)
{
	return (smidlibStart());
}

static void selFree(void)
{
	smidlibFree();
}

static int selGetNearClock(void)
{
	return(smidlibGetNearClock());
}

static bool selNextClock(bool ShowEventMessage,bool EnableNote,int DecClock)
{
	return(smidlibNextClock(ShowEventMessage,EnableNote,DecClock));
}

static void selAllSoundOff(void)
{
	smidlibAllSoundOff();
}

static bool sel_isAllTrackEOF(void)
{
	return(SM_isAllTrackEOF());
}

static u32 sel_GetSamplePerClockFix16(void)
{
	return(SM_GetSamplePerClockFix16());
}

static void Start_smidlibDetectTotalClock()
{
  TSM_Track *pSM_Track=NULL;
  u32 trklen=0;
  u32 idx=0;
  {
    for(idx=0;idx<StdMIDI.SM_Chank.Track;idx++){
      TSM_Track *pCurSM_Track=&StdMIDI.SM_Tracks[idx];
      u32 ctrklen=(u32)pCurSM_Track->DataEnd-(u32)pCurSM_Track->Data;
      if(trklen<ctrklen){
        trklen=ctrklen;
        pSM_Track=pCurSM_Track;
      }
    }
  }
  
  if((pSM_Track==NULL)||(trklen==0)){
    _consolePrintf("Fatal error.\n");
    
	// ShowLogHalt();	// This function cannot be implemented by us - KHS
  }
  

  // MWin_ProgressShow("Loading...",trklen);  // This function cannot be implemented by us - KHS
  
  int LastClock=0;
  int PrevDiv=StdMIDI.SM_Chank.TimeRes*8;
  
  while(1){
    if(PrevDiv<(TotalClock-LastClock)){
      LastClock=TotalClock;
      u32 lastlen=(u32)pSM_Track->DataEnd-(u32)pSM_Track->Data;
      
	  // if(lastlen<trklen) MWin_ProgressSetPos(trklen-lastlen);	// This function cannot be implemented by us - KHS
    }
    int DecClock=smidlibGetNearClock();
    if(smidlibNextClock(false,false,DecClock)==false) break;
    TotalClock+=DecClock;
  }
  
  // MWin_ProgressHide();	// // This function cannot be implemented by us - KHS
}

bool Start_InitStackBuf(void)
{
	u32 ch = 0;
	u32 sidx = 0;

	for(ch = 0;ch<ChannelsCount;ch++){
		for(sidx = 0;sidx<MaxStackCount;sidx++){
			pStackBuf[ch][sidx]=NULL;
		}
	}
  
  StackCount=GlobalINI.MIDPlugin.DelayStackSize;
  if(StackCount==0) StackCount=1;
  if(MaxStackCount<StackCount) StackCount=MaxStackCount;
  
  for(ch = 0;ch<ChannelsCount;ch++){
    for(sidx = 0;sidx<StackCount;sidx++){
      pStackBuf[ch][sidx]=(s32*)safemalloc((MaxSamplePerFrame+1)*2*4);
      if(pStackBuf[ch][sidx]==NULL){
        _consolePrintf("pStackBuf: Memory overflow.\n");
        return(false);
      }
      MemSet32CPU(0,pStackBuf[ch][sidx],(MaxSamplePerFrame+1)*2*4);
    }
    StackLastCount[ch]=0;
  }
  
  StackIndex=0;
  
  return(true);
}

bool Start(int FileHandle)
{
  InitINI();
  LoadINI(GetINIData(),GetINISize());
  
  if(Start_InitStackBuf()==false) return(false);
  
  TiniMIDPlugin *MIDPlugin=&GlobalINI.MIDPlugin;
  
  {
    if(MaxSampleRate<MIDPlugin->SampleRate) MIDPlugin->SampleRate=MaxSampleRate;
    
    SamplePerFrame=MIDPlugin->SampleRate/MIDPlugin->FramePerSecond;
    if(MaxSamplePerFrame<SamplePerFrame) SamplePerFrame=MaxSamplePerFrame;
    SamplePerFrame&=~15;
    if(SamplePerFrame<16) SamplePerFrame=16;
    
  }
  
  {
    u32 samples=SamplePerFrame+16;
    pReserve[0]=safemalloc(samples*8*2);
    pReserve[1]=safemalloc(samples*8*2);
    pReserve[2]=safemalloc(samples*2);
    pReserve[3]=safemalloc(samples*2);
  }
  
  FileSys_fseek(FileHandle,0,SEEK_END);
  DeflateSize=FileSys_ftell(FileHandle);
  FileSys_fseek(FileHandle,0,SEEK_SET);
  
  DeflateBuf=(u8*)safemalloc(DeflateSize);
  if(DeflateBuf==NULL) return(false);
  FileSys_fread(DeflateBuf,1,DeflateSize,FileHandle);
  
  if(GetBINFileHandle()==0){
    _consolePrintf("not found sound font file. 'midrcp.bin'\n");
    if(DeflateBuf!=NULL){
      safefree(DeflateBuf); DeflateBuf=NULL;
    }
    return(false);
  }
  
  PCH_SetProgramMap(GetBINFileHandle());
  
  {
    bool detect=false;
    
	// The following routine is not required for our program. - KHS
	/*
    if(strncmp((char*)DeflateBuf,"MThd",4)==0){
      _consolePrintf("Start Standard MIDI file.\n");
      detect=true;
      FileFormat=EFF_MID;
    }
    if(strncmp((char*)DeflateBuf,"RCM-PC98V2.0(C)COME ON MUSIC",28)==0){
      _consolePrintf("Start RecomposerV2.0 file.\n");
      detect=true;
      FileFormat=EFF_RCP;
    }
    */
	// Therefore, I arranged and copy that routine as below. - KHS

	if(strncmp((char*)DeflateBuf,"MThd",4)==0){
      _consolePrintf("Start Standard MIDI file.\n");
      detect=true; 
    }
    
    if(detect==false){
      _consolePrintf("Unknown file format!!\n");
      PCH_FreeProgramMap();
      if(DeflateBuf!=NULL){
        safefree(DeflateBuf); DeflateBuf=NULL;
      }
      return(false);
    }
  }

  selSetParam(DeflateBuf,MIDPlugin->SampleRate,SamplePerFrame,MIDPlugin->MaxVoiceCount,MIDPlugin->GenVolume);
  if(selStart()==false){
    Free();
    return(false);
  }
  
  TotalClock=0;
  CurrentClock=0;
  
  PrfStart();

  Start_smidlibDetectTotalClock();

  SoundFontLoadTimeus=PrfEnd(0);
  
  if(TotalClock==0){
    _consolePrintf("Detect TotalClock equal Zero.\n");
    Free();
    return(false);
  }
  
  selFree();
  selStart();
  
  ClockCur=selGetNearClock();
  selNextClock(MIDPlugin->ShowEventMessage,true,selGetNearClock());
  
  u32 idx=0;
  for(idx=0;idx<ReserveCount;idx++){
    if(pReserve[idx]!=NULL){
      safefree(pReserve[idx]); pReserve[idx]=NULL;
    }
  }
  
  return(true);
}

void Free(void)
{
  u32 idx=0;
  for(idx=0;idx<ReserveCount;idx++){
    if(pReserve[idx]!=NULL){
      safefree(pReserve[idx]); pReserve[idx]=NULL;
    }
  }
  
  selFree();
  
  PCH_FreeProgramMap();
  
  u32 ch=0;
  u32 sidx = 0;
  for(ch = 0;ch<ChannelsCount;ch++){
    for(sidx=0;sidx<MaxStackCount;sidx++){
      if(pStackBuf[ch][sidx]!=NULL){
        safefree(pStackBuf[ch][sidx]); pStackBuf[ch][sidx]=NULL;
      }
    }
  }
  
  if(DeflateBuf!=NULL){
    safefree(DeflateBuf); DeflateBuf=NULL;
  }
}

u32 Update(s16 *lbuf,s16 *rbuf)
{  
  if(sel_isAllTrackEOF()==true) return(0);
  
  TiniMIDPlugin *MIDPlugin=&GlobalINI.MIDPlugin;
  
  int ProcClock=0;
  
  {
    u32 SamplePerFrameFix16=SamplePerFrame*0x10000;
    u32 SamplePerClockFix16=sel_GetSamplePerClockFix16();
    while(ClockCur<SamplePerFrameFix16){
      ProcClock++;
      ClockCur+=SamplePerClockFix16;
    }
    ClockCur-=SamplePerFrame*0x10000;
  }
  
  while(ProcClock!=0){
    int DecClock=selGetNearClock();
//    if(ProcClock>=DecClock) _consolePrintf("[%d]",CurrentClock+1);
    if(ProcClock<DecClock) DecClock=ProcClock;
    ProcClock-=DecClock;
    CurrentClock+=DecClock;
    if(selNextClock(MIDPlugin->ShowEventMessage,true,DecClock)==false) break;
  }
  
  PCH_NextClock();
  
  PCH_RenderStart(SamplePerFrame);
  
  if((lbuf!=NULL)&&(rbuf!=NULL)){
    MemSet32CPU(0,DTCM_dstbuf,SamplePerFrame*2*4);
    
    StackIndex++;
    if(StackIndex==StackCount) StackIndex=0;
    
	u32 ch=0;
    for(ch = 0;ch<ChannelsCount;ch++){
      bool reqproc=false;
      bool reqrender=false;
      if(StackLastCount[ch]!=0) reqproc=true;
      if(PCH_RequestRender(ch)==true){
        reqproc=true;
        reqrender=true;
      }
      
      if(reqproc==true){
        s32 *ptmpbuf=DTCM_tmpbuf;
        
        MemSet32CPU(0,ptmpbuf,SamplePerFrame*2*4);
        
        if(reqrender==true){
          PCH_Render(ch,ptmpbuf,SamplePerFrame);
          StackLastCount[ch]=StackCount*4;
          }else{
          StackLastCount[ch]--;
        }
        
        s32 *pstackbuf=pStackBuf[ch][StackIndex];
        
        {
          s32 *p=&pstackbuf[(SamplePerFrame-1)*2];
          asm volatile(
            "ldmia %0!,{r0,r1} \n"
            "stmia %0,{r0,r1} \n"
            "sub %0,#2*4 \n"
            : : "r"(p)
            : "r0","r1"
          );
        }
        
        s32 *pdstbuf=DTCM_dstbuf;
        
        u32 ReverbFactor;
        
        if(PCH_isDrumMap(ch)==true){
          ReverbFactor=GlobalINI.MIDPlugin.ReverbFactor_DrumMap;
          }else{
          ReverbFactor=GlobalINI.MIDPlugin.ReverbFactor_ToneMap;
        }
        
        vu32 Reverb=PCH_GetReverb(ch)*ReverbFactor; // 127*127=14bit
        Reverb+=16*128;
        Reverb<<=1; // 14bit -> 15bit
        if(0x6000<Reverb) Reverb=0x6000; // limited
        
        asm volatile(
          // %0=SamplePerFrame
          // %1=pstackbuf
          // %2=ptmpbuf
          // %3=Reverb
          // %4=pdstbuf
          
          // r0,r1,r2,r3=pstackbuf l/r/l/r
          // r4,r5=ptmpbuf l/r
          // r10=SamplePerFrame-1
          
          "mov r10,%0 \n"
          
          "reverb_loop: \n"
          
          "ldmia %1,{r0,r1,r2,r3} \n"
          "ldmia %2!,{r4,r5} \n"
          
          "add r0,r2 \n"
          "smulwb r2,r0,%3 \n"
          "add r1,r3 \n"
          "smulwb r3,r1,%3 \n"
          "add r1,r2,r5 \n"
          "add r0,r3,r4 \n"
          
          "stmia %1!,{r0,r1} \n"
          
          "ldmia %4,{r4,r5} \n"
          "add r4,r0 \n"
          "add r5,r1 \n"
          "stmia %4!,{r4,r5} \n"
          
          "subs %0,#1 \n"
          "bne reverb_loop \n"
          
          "mov r10,%0 \n"
          
          "sub %1,r10,lsl #3 \n"
          "sub %2,r10,lsl #3 \n"
          "sub %4,r10,lsl #3 \n"
          
          : : "r"(SamplePerFrame),"r"(pstackbuf),"r"(ptmpbuf),"r"(Reverb),"r"(pdstbuf)
          : "r0","r1","r2","r3","r4","r5","r10"
        );
      }
    }
    
    asm volatile(
      "push {%0} \n"
      
      "ldr r8,=-0x8000 \n"
      "ldr r9,=0xffff \n"
      
      "copybuffer_stereo: \n"
      
      "ldmia %1!,{r0,r1,r2,r3,r4,r5,r6,r7} \n"
      
      "cmps r0,r8 \n movlt r0,r8 \n cmps r0,r9,lsr #1 \n movgt r0,r9,lsr #1 \n"
      "cmps r1,r8 \n movlt r1,r8 \n cmps r1,r9,lsr #1 \n movgt r1,r9,lsr #1 \n"
      "cmps r2,r8 \n movlt r2,r8 \n cmps r2,r9,lsr #1 \n movgt r2,r9,lsr #1 \n"
      "cmps r3,r8 \n movlt r3,r8 \n cmps r3,r9,lsr #1 \n movgt r3,r9,lsr #1 \n"
      "cmps r4,r8 \n movlt r4,r8 \n cmps r4,r9,lsr #1 \n movgt r4,r9,lsr #1 \n"
      "cmps r5,r8 \n movlt r5,r8 \n cmps r5,r9,lsr #1 \n movgt r5,r9,lsr #1 \n"
      "cmps r6,r8 \n movlt r6,r8 \n cmps r6,r9,lsr #1 \n movgt r6,r9,lsr #1 \n"
      "cmps r7,r8 \n movlt r7,r8 \n cmps r7,r9,lsr #1 \n movgt r7,r9,lsr #1 \n"
      
      "and r0,r9 \n"
      "orr r0,r0,r2,lsl #16 \n"
      "and r4,r9 \n"
      "orr r4,r4,r6,lsl #16 \n"
      "stmia %2!,{r0,r4} \n"
      
      "and r1,r9 \n"
      "orr r1,r1,r3,lsl #16 \n"
      "and r5,r9 \n"
      "orr r5,r5,r7,lsl #16 \n"
      "stmia %3!,{r1,r5} \n"
      
      "subs %0,#4 \n"
      "bne copybuffer_stereo \n"
      
      "pop {%0} \n"
      
      : : "r"(SamplePerFrame),"r"(DTCM_dstbuf),"r"(lbuf),"r"(rbuf)
      : "r0","r1","r2","r3","r4","r5","r6","r7", "r8","r9"
    );
  }
  
  PCH_RenderEnd();
  
  return(SamplePerFrame);
}

s32 GetPosMax(void)
{
  return(TotalClock);
}

s32 GetPosOffset(void)
{
  return(CurrentClock);
}

void SetPosOffset(s32 ofs)
{
  if(ofs<CurrentClock){
    selFree();
    selStart();
    CurrentClock=0;
    }else{
    selAllSoundOff();
  }
  
  while(CurrentClock<=ofs){
    int DecClock=selGetNearClock();
    if(selNextClock(false,false,DecClock)==false) break;
    CurrentClock+=DecClock;
  }
  
  ClockCur=0;
}

bool GetInfoStrW(int idx, u16 *str,int len)
{
  return(false);
}

bool GetInfoStrUTF8(int idx,char *str,int len)
{
  return(false);
}