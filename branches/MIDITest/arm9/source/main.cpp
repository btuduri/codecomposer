#include "nds.h"

#include <nds/arm9/console.h> 
#include <stdio.h>
#include <string.h>

#include "std.h"
#include "inifile.h"
#include "smidlib.h"
#include "smidlib_pch.h"
#include "smidlib_sm.h"

#include "_const.h"
#include "memtool.h"
#include "filesys.h"
#include "plugin_supple.h"
#include "shell.h"
#include "cimfs.h"

#define ChannelsCount (16)
#define MaxSampleRate (32768)
#define MinFramePerSecond (120)
#define MaxSamplePerFrame (MaxSampleRate/MinFramePerSecond)
#define MaxStackCount (12)
#define ReserveCount (4)

u8 *DeflateBuf = 0;
u32 DeflateSize;
u32 SamplePerFrame;
int TotalClock,CurrentClock;
u32 ClockCur;
ALIGNED_VAR_IN_DTCM s32 DTCM_tmpbuf[MaxSamplePerFrame*2];
ALIGNED_VAR_IN_DTCM s32 DTCM_dstbuf[MaxSamplePerFrame*2];

u32 StackIndex;
u32 StackCount;
s32 *pStackBuf[ChannelsCount][MaxStackCount];
s32 StackLastCount[ChannelsCount];
void *pReserve[ReserveCount];
u32 SoundFontLoadTimeus;

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

int main(void) 
{
	touchPosition touchXY;

	irqInit();
	irqEnable(IRQ_VBLANK);

	videoSetMode(0);	//not using the main screen
	videoSetModeSub(MODE_0_2D | DISPLAY_BG0_ACTIVE);	//sub bg 0 will be used to print text
	vramSetBankC(VRAM_C_SUB_BG);

	SUB_BG0_CR = BG_MAP_BASE(31);

	BG_PALETTE_SUB[255] = RGB15(31,31,31);	//by default font will be rendered with color 255
	consoleInitDefault((u16*)SCREEN_BASE_BLOCK_SUB(31), (u16*)CHAR_BASE_BLOCK_SUB(0), 16);

    pIMFS = new CIMFS();
    if(pIMFS->InitIMFS()==false) 
		while(1);

    FileSys_Init(256);
    Shell_AutoDetect();
  
    if(Shell_FindShellPath()==false) 
		while(1);

	int file_handle = 0;
	file_handle = Shell_OpenFile("Dancing_Queen.mid");

	Start(file_handle);

	while(1) {

		touchXY=touchReadXY();
		iprintf("\x1b[10;0HTouch x = %04X, %04X\n", touchXY.x, touchXY.px);
		iprintf("Touch y = %04X, %04X\n", touchXY.y, touchXY.py);

		swiWaitForVBlank();
	}

	return 0;
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



bool Start(int FileHandle)
{
 TPluginBody *pPB=(TPluginBody*)safemalloc(sizeof(TPluginBody));
 pPB->INIData=NULL;
 pPB->INISize=0;
 pPB->BINFileHandle=0;
 pPB->BINData=NULL;
 pPB->BINSize=0;

 InitINI();
 LoadINI(GetINIData(),GetINISize());
 
 iprintf("stage1 is passed\n");
 if(Start_InitStackBuf()==false) 
	 return(false);
 
 TiniMIDPlugin *MIDPlugin=&MIDIINI.MIDPlugin;
 
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
 
 fseek((FILE*)FileHandle,0,SEEK_END);
 DeflateSize=ftell((FILE*)FileHandle);
 fseek((FILE*)FileHandle,0,SEEK_SET);
 
 DeflateBuf=(u8*)safemalloc(DeflateSize);
 
 iprintf("deflatesize is %d\n", DeflateSize);
 if(DeflateBuf==NULL) 
	 return(false);
 

 iprintf("stage3 is passed\n");
 fread(DeflateBuf,1,DeflateSize,(FILE*)FileHandle);
 
 if(GetBINFileHandle()==0){
   iprintf("not found sound font file. 'midrcp.bin'\n");
   if(DeflateBuf!=NULL){
     safefree(DeflateBuf); DeflateBuf=NULL;
   }
   return(false);
 }
 
 iprintf("stage4 is passed\n");
 PCH_SetProgramMap(GetBINFileHandle());
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
 
 iprintf("stage5 is passed\n");

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

int GetInfoIndexCount(void)
{
 
	return(5);
}

bool GetInfoStrL(int idx,char *str,int len)
{
 if(MIDIINI.MIDPlugin.ShowInfomationMessages==true){
   if(GetBINSize()<1*1024*1024){
     switch(idx){
     }
     return(false);
   }
   
   if(MemoryOverflowFlag==true){
     switch(idx){
     }
     return(false);
   }
 }
 
 return(false);
}
