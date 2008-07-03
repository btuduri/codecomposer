
#include <stdio.h>

#include <nds.h>

#include "_const.h"

#include "plugin.h"
#include "plugin_def.h"
#include "std.h"

#include "inifile.h"

#include "smidlib.h"
#include "rcplib.h"

#include "shell.h"


// CallBack from plugin_dll.cpp

void cbLoadLibrary(void)
{
}

void cbFreeLibrary(void)
{
}

void cbQueryInterfaceLibrary(void)
{
}

// -----------------

static u8 *DeflateBuf=NULL;
static u32 DeflateSize;

static u32 SamplePerFrame;

static int TotalClock,CurrentClock;

static u32 ClockCur;

enum EFileFormat {EFF_MID,EFF_RCP};

static EFileFormat FileFormat;

#define MaxSampleRate (32768)
#define MinFramePerSecond (120)
#define MaxSamplePerFrame (MaxSampleRate/MinFramePerSecond)

static ALIGNED_VAR_IN_DTCM s32 DTCM_tmpbuf[MaxSamplePerFrame*2];
static ALIGNED_VAR_IN_DTCM s32 DTCM_dstbuf[MaxSamplePerFrame*2];

static u32 StackIndex;
static u32 StackCount;
#define MaxStackCount (12)
static s32 *pStackBuf[ChannelsCount][MaxStackCount];
static s32 StackLastCount[ChannelsCount];

// --------------------------------------------------------------------

// ------------------------------------------------------------------------------------

void Free(void);

static void selSetParam(u8 *data,u32 SampleRate,u32 SampleBufCount,u32 MaxChannelCount,u32 GenVolume)
{
  switch(FileFormat){
//    case EFF_MID: smidlibSetParam(DeflateBuf,SampleRate,SampleBufCount,MaxChannelCount,GenVolume); break;
//    case EFF_RCP: rcplibSetParam(DeflateBuf,SampleRate,SampleBufCount,MaxChannelCount,GenVolume); break;
  }
}

static bool selStart(void)
{
  switch(FileFormat){
    case EFF_MID: return(smidlibStart()); break;
    case EFF_RCP: return(rcplibStart()); break;
  }
  
  return(false);
}

static void selFree(void)
{
  switch(FileFormat){
    case EFF_MID: smidlibFree(); break;
    case EFF_RCP: rcplibFree(); break;
  }
}

static int selGetNearClock(void)
{
  switch(FileFormat){
    case EFF_MID: return(smidlibGetNearClock()); break;
    case EFF_RCP: return(rcplibGetNearClock()); break;
  }
  
  return(0);
}

static bool selNextClock(bool ShowEventMessage,bool EnableNote,int DecClock)
{
  switch(FileFormat){
    case EFF_MID: return(smidlibNextClock(ShowEventMessage,EnableNote,DecClock)); break;
    case EFF_RCP: return(rcplibNextClock(ShowEventMessage,EnableNote,DecClock)); break;
  }
  
  return(false);
}

static void selAllSoundOff(void)
{
  switch(FileFormat){
    case EFF_MID: smidlibAllSoundOff(); break;
    case EFF_RCP: rcplibAllSoundOff(); break;
  }
}

static bool sel_isAllTrackEOF(void)
{
  switch(FileFormat){
    case EFF_MID: return(SM_isAllTrackEOF()); break;
    case EFF_RCP: return(RCP_isAllTrackEOF()); break;
  }
  
  return(false);
}

static u32 sel_GetSamplePerClockFix16(void)
{
  switch(FileFormat){
    case EFF_MID: return(SM_GetSamplePerClockFix16()); break;
    case EFF_RCP: return(RCP_GetSamplePerClockFix16()); break;
  }
  
  return(0);
}

static void Start_smidlibDetectTotalClock()
{
  TSM_Track *pSM_Track=NULL;
  u32 trklen=0;
  
  {
    for(u32 idx=0;idx<StdMIDI.SM_Chank.Track;idx++){
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
    ShowLogHalt();
  }
  
  MWin_ProgressShow("Loading...",trklen);
  
  int LastClock=0;
  int PrevDiv=StdMIDI.SM_Chank.TimeRes*8;
  
  while(1){
    if(PrevDiv<(TotalClock-LastClock)){
      LastClock=TotalClock;
      u32 lastlen=(u32)pSM_Track->DataEnd-(u32)pSM_Track->Data;
      if(lastlen<trklen) MWin_ProgressSetPos(trklen-lastlen);
    }
    int DecClock=smidlibGetNearClock();
    if(smidlibNextClock(false,false,DecClock)==false) break;
    TotalClock+=DecClock;
  }
  
  MWin_ProgressHide();
}

static void Start_rcplibDetectTotalClock()
{
  TRCP_Track *pRCP_Track=NULL;
  u32 trklen=0;
  
  {
    for(u32 idx=0;idx<RCP_Chank.TrackCount;idx++){
      TRCP_Track *pCurRCP_Track=&RCP.RCP_Track[idx];
      u32 ctrklen=(u32)pCurRCP_Track->DataEnd-(u32)pCurRCP_Track->Data;
      if(trklen<ctrklen){
        trklen=ctrklen;
        pRCP_Track=pCurRCP_Track;
      }
    }
  }
  
  if((pRCP_Track==NULL)||(trklen==0)){
    _consolePrintf("Fatal error.\n");
    ShowLogHalt();
  }
  
  MWin_ProgressShow("Loading...",trklen);
  
  int LastClock=0;
  int PrevDiv=RCP_Chank.TimeRes*8;
  
  while(1){
    if(PrevDiv<(TotalClock-LastClock)){
      LastClock=TotalClock;
      u32 lastlen=(u32)pRCP_Track->DataEnd-(u32)pRCP_Track->Data;
      if(lastlen<trklen) MWin_ProgressSetPos(trklen-lastlen);
    }
    int DecClock=rcplibGetNearClock();
    if(rcplibNextClock(false,false,DecClock)==false) break;
    TotalClock+=DecClock;
  }
  
  MWin_ProgressHide();
}

bool Start_InitStackBuf(void)
{
  for(u32 ch=0;ch<ChannelsCount;ch++){
    for(u32 sidx=0;sidx<MaxStackCount;sidx++){
      pStackBuf[ch][sidx]=NULL;
    }
  }
  
  StackCount=GlobalINI.MIDPlugin.DelayStackSize;
  if(StackCount==0) StackCount=1;
  if(MaxStackCount<StackCount) StackCount=MaxStackCount;
  
  for(u32 ch=0;ch<ChannelsCount;ch++){
    for(u32 sidx=0;sidx<StackCount;sidx++){
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

#define ReserveCount (4)
static void *pReserve[ReserveCount];

static u32 SoundFontLoadTimeus;

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
  
  fseek(FileHandle,0,SEEK_END);
  DeflateSize=ftell(FileHandle);
  fseek(FileHandle,0,SEEK_SET);
  
  DeflateBuf=(u8*)safemalloc(DeflateSize);
  if(DeflateBuf==NULL) return(false);
  fread(DeflateBuf,1,DeflateSize,FileHandle);
  
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
  switch(FileFormat){
    case EFF_MID: Start_smidlibDetectTotalClock(); break;
    case EFF_RCP: Start_rcplibDetectTotalClock(); break;
  }
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
  
  for(u32 idx=0;idx<ReserveCount;idx++){
    if(pReserve[idx]!=NULL){
      safefree(pReserve[idx]); pReserve[idx]=NULL;
    }
  }
  
  return(true);
}

void Free(void)
{
  for(u32 idx=0;idx<ReserveCount;idx++){
    if(pReserve[idx]!=NULL){
      safefree(pReserve[idx]); pReserve[idx]=NULL;
    }
  }
  
  selFree();
  
  PCH_FreeProgramMap();
  
  for(u32 ch=0;ch<ChannelsCount;ch++){
    for(u32 sidx=0;sidx<MaxStackCount;sidx++){
      if(pStackBuf[ch][sidx]!=NULL){
        safefree(pStackBuf[ch][sidx]); pStackBuf[ch][sidx]=NULL;
      }
    }
  }
  
  if(DeflateBuf!=NULL){
    safefree(DeflateBuf); DeflateBuf=NULL;
  }
}
#if 0
u32 Update(s16 *lbuf,s16 *rbuf)
{
//  _consolePrintf("SoundFontLoadTimeus=%dus\n",SoundFontLoadTimeus);

//  PrfStart();
  
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
    
    for(u32 ch=0;ch<ChannelsCount;ch++){
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
  
/*
  static u32 a=0;
  if(a<256){
    a++;
    }else{
    ShowLogHalt();
  }
*/
  
//  PrfEnd(0);
  
  return(SamplePerFrame);
}
#endif

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

u32 GetChannelCount(void)
{
  return(2);
}

u32 GetSampleRate(void)
{
  TiniMIDPlugin *MIDPlugin=&GlobalINI.MIDPlugin;
  
  return(MIDPlugin->SampleRate);
}

u32 GetSamplePerFrame(void)
{
  return(SamplePerFrame);
}

int GetInfoIndexCount(void)
{
  if(GlobalINI.MIDPlugin.ShowInfomationMessages==true){
    if(GetBINSize()<1*1024*1024) return(5);
    if(MemoryOverflowFlag==true) return(5);
  }
  
  switch(FileFormat){
    case EFF_MID: return(5); break;
    case EFF_RCP: return(15); break;
  }
  
  return(0);
}

bool GetInfoStrL(int idx,char *str,int len)
{
  if(GlobalINI.MIDPlugin.ShowInfomationMessages==true){
    if(GetBINSize()<1*1024*1024){
      switch(idx){
        case 0: snprintf(str,len,"This sound font is very compact."); return(true); break;
        case 1: snprintf(str,len,"Install full sound font from web site."); return(true); break;
        case 2: snprintf(str,len,"これは小さなサウンドフォントです。"); return(true); break;
        case 3: snprintf(str,len,"ウェブサイトから完全版をDLできます。"); return(true); break;
        case 4: snprintf(str,len,"http://mdxonline.dyndns.org/"); return(true); break;
      }
      return(false);
    }
    
    if(MemoryOverflowFlag==true){
      switch(idx){
        case 0: snprintf(str,len,"Sound font memory overflow."); return(true);
        case 1: snprintf(str,len,"Please try 8bit sound font."); return(true);
        case 2: snprintf(str,len,"PCMメモリが足りませんでした。"); return(true);
        case 3: snprintf(str,len,"詳しくはサウンドフォント付属の"); return(true);
        case 4: snprintf(str,len,"テキストを参照してください。"); return(true);
      }
      return(false);
    }
  }
  
  switch(FileFormat){
    case EFF_MID: {
      TSM_Chank *_SM_Chank=&StdMIDI.SM_Chank;
      switch(idx){
        case 0: snprintf(str,len,"Format=%d Tracks=%d TimeRes=%d",_SM_Chank->Format,_SM_Chank->Track,_SM_Chank->TimeRes); return(true); break;
        case 1: snprintf(str,len,"Title=%s",(gME_Title!=NULL) ? gME_Title : ""); return(true); break;
        case 2: snprintf(str,len,"Copyright=%s",(gME_Copyright!=NULL) ? gME_Copyright : ""); return(true); break;
        case 3: snprintf(str,len,"Text=%s",(gME_Text!=NULL) ? gME_Text : ""); return(true); break;
        case 4: snprintf(str,len,"TotalClock=%d",TotalClock); return(true); break;
      }
    } break;
    case EFF_RCP: {
      TRCP_Chank *pRCP_Chank=&RCP_Chank;
      switch(idx){
        case 0: snprintf(str,len,"TimeRes=%d Tempo=%d PlayBias=%d",pRCP_Chank->TimeRes,pRCP_Chank->Tempo,pRCP_Chank->PlayBias); return(true); break;
        case 1: snprintf(str,len,"TrackCount=%d TotalClock=%d",pRCP_Chank->TrackCount,TotalClock); return(true); break;
        case 2: snprintf(str,len,"%0.64s",pRCP_Chank->Title); return(true); break;
        case 3: snprintf(str,len,"%0.28s",&pRCP_Chank->Memo[0*28]); return(true); break;
        case 4: snprintf(str,len,"%0.28s",&pRCP_Chank->Memo[1*28]); return(true); break;
        case 5: snprintf(str,len,"%0.28s",&pRCP_Chank->Memo[2*28]); return(true); break;
        case 6: snprintf(str,len,"%0.28s",&pRCP_Chank->Memo[3*28]); return(true); break;
        case 7: snprintf(str,len,"%0.28s",&pRCP_Chank->Memo[4*28]); return(true); break;
        case 8: snprintf(str,len,"%0.28s",&pRCP_Chank->Memo[5*28]); return(true); break;
        case 9: snprintf(str,len,"%0.28s",&pRCP_Chank->Memo[6*28]); return(true); break;
        case 10: snprintf(str,len,"%0.28s",&pRCP_Chank->Memo[7*28]); return(true); break;
        case 11: snprintf(str,len,"%0.28s",&pRCP_Chank->Memo[8*28]); return(true); break;
        case 12: snprintf(str,len,"%0.28s",&pRCP_Chank->Memo[9*28]); return(true); break;
        case 13: snprintf(str,len,"%0.28s",&pRCP_Chank->Memo[10*28]); return(true); break;
        case 14: snprintf(str,len,"%0.28s",&pRCP_Chank->Memo[11*28]); return(true); break;
      }
    } break;
  }
  
  return(false);
}

bool GetInfoStrW(int idx,UnicodeChar *str,int len)
{
  return(false);
}

bool GetInfoStrUTF8(int idx,char *str,int len)
{
  return(false);
}

// -----------------------------------------------------------

volatile int frame = 0;

//---------------------------------------------------------------------------------
void Vblank() {
//---------------------------------------------------------------------------------
	frame++;
}

int main()
{
	int file_handle;
	file_handle = Shell_SkinOpenFile("/d/Source/NDS/JinShell/JinShell/Dancing_Queen.mid");

	Start(file_handle);

	touchPosition touchXY;

	irqInit();
	irqSet(IRQ_VBLANK, Vblank);
	irqEnable(IRQ_VBLANK);
	videoSetMode(0);	//not using the main screen
	videoSetModeSub(MODE_0_2D | DISPLAY_BG0_ACTIVE);	//sub bg 0 will be used to print text
	vramSetBankC(VRAM_C_SUB_BG); 

	SUB_BG0_CR = BG_MAP_BASE(31);
	
	BG_PALETTE_SUB[255] = RGB15(31,31,31);	//by default font will be rendered with color 255
	
	//consoleInit() is a lot more flexible but this gets you up and running quick
	consoleInitDefault((u16*)SCREEN_BASE_BLOCK_SUB(31), (u16*)CHAR_BASE_BLOCK_SUB(0), 16);

	iprintf("      Hello DS dev'rs\n");
	iprintf("     www.devkitpro.org\n");
	iprintf("   www.drunkencoders.com");

	while(1) {
	
		swiWaitForVBlank();
		touchXY=touchReadXY();

		// print at using ansi escape sequence \x1b[line;columnH 
		iprintf("\x1b[10;0HFrame = %d",frame);
		iprintf("\x1b[16;0HTouch x = %04X, %04X\n", touchXY.x, touchXY.px);
		iprintf("Touch y = %04X, %04X\n", touchXY.y, touchXY.py);		
	
	}


	return 0;
}
