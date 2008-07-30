// files from moonshell
#include <nds.h>
#include <nds/arm9/console.h>
#include <stdio.h>
#include <string.h>

#include "inifile.h"
#include "smidlib.h"
#include "smidlib_pch.h"
#include "smidlib_sm.h"
#include "smidlib_mtrk.h"
#include "_const.h"
#include "memtool.h"
#include "filesys.h"
#include "shell.h"
#include "cimfs.h"
#include "directdisk.h"
#include "../../ipc3.h"
#include "strpcm.h"
#include "extmem.h"

#include "smidproc.h"

u8 *DeflateBuf = 0;
u32 DeflateSize;
u32 SamplePerFrame;
int TotalClock,CurrentClock;
u32 ClockCur;
ALIGNED_VAR_IN_DTCM s32 DTCM_tmpbuf[MaxSamplePerFrame*2];
ALIGNED_VAR_IN_DTCM s32 DTCM_dstbuf[MaxSamplePerFrame*2];
u32 SetARM9_REG_ROM1stAccessCycleControl=0;
u32 SetARM9_REG_ROM2stAccessCycleControl=0;
u32 StackIndex;
u32 StackCount;
s32 *pStackBuf[ChannelsCount][MaxStackCount];
s32 StackLastCount[ChannelsCount];
void *pReserve[ReserveCount];
u32 SoundFontLoadTimeus;
TPluginBody *pPB;
TiniMIDPlugin *MIDPlugin;

extern s16 *strpcmRingLBuf;
extern s16 *strpcmRingRBuf;

// functions from moonshell
bool initSmidlib()
{
 pPB = (TPluginBody*)safemalloc(sizeof(TPluginBody));
 MIDPlugin = &MIDIINI.MIDPlugin;

 pPB->INIData=NULL;
 pPB->INISize=0;
 pPB->BINFileHandle=0;
 pPB->BINData=NULL;
 pPB->BINSize=0;

 InitINI();
 LoadINI(GetINIData(pPB),GetINISize(pPB));
 
 if(Start_InitStackBuf()==false) 
	 return(false);
 
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

 if(!loadSoundSource(pPB))
	 return false;

 SM_Init();
 SM_ProcMetaEvent_InitTempo();
 PCH_Init(MIDPlugin->SampleRate,SamplePerFrame,MIDPlugin->MaxVoiceCount,MIDPlugin->GenVolume);
 MTRKCC_Init();

 u32 idx=0;
 for(idx=0;idx<ReserveCount;idx++){
   if(pReserve[idx]!=NULL){
     safefree(pReserve[idx]); pReserve[idx]=NULL;
   }
 }
 
 return true;
}

void selSetParam(u8 *data,u32 SampleRate,u32 SampleBufCount,u32 MaxChannelCount,u32 GenVolume)
{
	smidlibSetParam(DeflateBuf,SampleRate,SampleBufCount,MaxChannelCount,GenVolume);
}

bool selStart(void)
{
	return(smidlibStart());
}

void selFree(void)
{
	smidlibFree(); 
}

 int selGetNearClock(void)
{
	return(smidlibGetNearClock());
}

 bool selNextClock(bool ShowEventMessage,bool EnableNote,int DecClock)
{
	return(smidlibNextClock(ShowEventMessage,EnableNote,DecClock));
}

 void selAllSoundOff(void)
{
	smidlibAllSoundOff();
}

 bool sel_isAllTrackEOF(void)
{
	return(SM_isAllTrackEOF());
}

 u32 sel_GetSamplePerClockFix16(void)
{
	return(SM_GetSamplePerClockFix16());
}

 void Start_smidlibDetectTotalClock()
{
 TSM_Track *pSM_Track=NULL;
 u32 trklen=0;
 
 {
   u32 idx=0;
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
   iprintf("Fatal error.\n");
 }
 
 int LastClock=0;
 int PrevDiv=StdMIDI.SM_Chank.TimeRes*8;
 
 while(1){
   if(PrevDiv<(TotalClock-LastClock)){
     LastClock=TotalClock;
     u32 lastlen=(u32)pSM_Track->DataEnd-(u32)pSM_Track->Data;
   }

   int DecClock=smidlibGetNearClock();
   if(smidlibNextClock(false,false,DecClock)==false) break;
   TotalClock+=DecClock;
 }
}

bool Start_InitStackBuf(void)
{
 u32 ch=0;
 u32 sidx=0;
 for(ch=0;ch<ChannelsCount;ch++){
   for(sidx=0;sidx<MaxStackCount;sidx++){
     pStackBuf[ch][sidx]=NULL;
   }
 }
 
 StackCount=MIDIINI.MIDPlugin.DelayStackSize;
 if(StackCount==0) StackCount=1;
 if(MaxStackCount<StackCount) StackCount=MaxStackCount;
 
 for(ch=0;ch<ChannelsCount;ch++){
   for(sidx=0;sidx<StackCount;sidx++){
     pStackBuf[ch][sidx]=(s32*)safemalloc((MaxSamplePerFrame+1)*2*4);
     if(pStackBuf[ch][sidx]==NULL){
       iprintf("pStackBuf: Memory overflow.\n");
       return(false);
     }
     MemSet32CPU(0,pStackBuf[ch][sidx],(MaxSamplePerFrame+1)*2*4);
   }
   StackLastCount[ch]=0;
 }
 
 StackIndex=0;
 
 return(true);
}

bool loadSoundSource(TPluginBody *pPB)
{
 int res_binfilehandle = GetBINFileHandle(pPB);
 if(res_binfilehandle == 0)
 {
   iprintf("not found sound font file. 'midrcp.bin'\n");
   
   if(DeflateBuf!=NULL)
   {
     safefree(DeflateBuf); 
	 DeflateBuf=NULL;
   }

   return false;
 }
 
 PCH_SetProgramMap(res_binfilehandle);
 
 return true;
}

bool Start(int FileHandle)
{
 pPB->INIData=NULL;
 pPB->INISize=0;
 pPB->BINFileHandle=0;
 pPB->BINData=NULL;
 pPB->BINSize=0;

 InitINI();
 LoadINI(GetINIData(pPB),GetINISize(pPB));
 
 if(Start_InitStackBuf()==false) 
	 return(false);
 
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
 DeflateSize= FileSys_ftell(FileHandle);
 FileSys_fseek(FileHandle,0,SEEK_SET);
 
 DeflateBuf=(u8*)safemalloc(DeflateSize);
 
 iprintf("deflatesize is %d\n", DeflateSize);
 if(DeflateBuf==NULL) 
	 return(false);
 
 FileSys_fread(DeflateBuf,1,DeflateSize, FileHandle);
 
 if(!loadSoundSource(pPB))
	 return false;
 
 selSetParam(DeflateBuf,MIDPlugin->SampleRate,SamplePerFrame,MIDPlugin->MaxVoiceCount,MIDPlugin->GenVolume);
 
 if(selStart()==false){
   Free();
   return(false);
 }
 
 TotalClock=0;
 CurrentClock=0;
 
 Start_smidlibDetectTotalClock();
 
 if(TotalClock==0){
   iprintf("Detect TotalClock equal Zero.\n");
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
 u32 sidx=0;

 for(ch=0;ch<ChannelsCount;ch++){
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
 TiniMIDPlugin *MIDPlugin=&MIDIINI.MIDPlugin;
 
 return(MIDPlugin->SampleRate);
}

u32 GetSamplePerFrame(void)
{
 return(SamplePerFrame);
}

u32 Update(s16 *lbuf,s16 *rbuf)
{
  /*
  if(sel_isAllTrackEOF()==true) 
	  return(0);
  */

  /*
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
    if(ProcClock<DecClock) DecClock=ProcClock;
    ProcClock-=DecClock;
    CurrentClock+=DecClock;
    if(selNextClock(MIDPlugin->ShowEventMessage,true,DecClock)==false) break;
  }
  */

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
		  
		  iprintf("ptmpbuf is %d\n", *ptmpbuf);

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

          ReverbFactor = MIDPlugin->ReverbFactor_DrumMap;
          }else{
          ReverbFactor = MIDPlugin->ReverbFactor_ToneMap;
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

bool strpcmUpdate_mainloop(void)
{ 
  u32 BaseSamples=IPC3->strpcmSamples;
  u32 Samples=0;
  
  REG_IME=0;
  u32 CurIndex=(strpcmRingBufWriteIndex+1) & strpcmRingBufBitMask;
  u32 PlayIndex=strpcmRingBufReadIndex;
  bool EmptyFlag;

  EmptyFlag=strpcmRingEmptyFlag;
  strpcmRingEmptyFlag = false;
  REG_IME=1;
  
  iprintf("strpcmRingBufWriteIndex is %d\n", strpcmRingBufWriteIndex);
  iprintf("strpcmRingBufReadIndex is %d\n", strpcmRingBufReadIndex);
  iprintf("loop:IR is %d\n", IPC3->IR);
  iprintf("loop:strpcmWriteRequest is %d\n", IPC3->strpcmWriteRequest);

  // Why these things happen!
  if(CurIndex==PlayIndex) 
  {
	iprintf("CurIndex and PlayIndex are the same\n");	
	return (false);
  }
	
  if(EmptyFlag==true)
  	 iprintf("strpcm:CPU overflow.\n");

  if((strpcmRingLBuf==NULL)||(strpcmRingRBuf==NULL)) 
  {
	  iprintf("mainloop Buffer problem\n");
	  return (false);
  }

  s16 *ldst=&strpcmRingLBuf[BaseSamples*CurIndex];
  s16 *rdst=&strpcmRingRBuf[BaseSamples*CurIndex];
  
  if(strpcmRequestStop==true)
  { 
    Samples=0;
  }
  else
  { 
    Samples = Update(ldst,rdst);
    
	if(Samples!=BaseSamples) 
		strpcmRequestStop=true;

	/*
	// 현재 Lbuf와 Rbuf가 0으로 나오는것을 봐서 버퍼에 내용이 복사가 안되므로 소리가 안나는것 같습니다.
	iprintf("Samples is %d\n", Samples);
	iprintf("Lbuf : %d\n", strpcmRingLBuf[BaseSamples*CurIndex]);
    iprintf("Rbuf : %d\n", strpcmRingRBuf[BaseSamples*CurIndex]);
	*/
  }
  
  if(Samples<BaseSamples)
  { 
    for(u32 idx=Samples;idx<BaseSamples;idx++)
	{ 
      ldst[idx]=0;
      rdst[idx]=0;
    }
  }
    
  REG_IME=0;
  strpcmRingBufWriteIndex=CurIndex;
  REG_IME=1;
  
  if(Samples==0) 
  {
	  iprintf("samples is %d, and return false\n", Samples);
	  return(false);
  }

  return(true);
}