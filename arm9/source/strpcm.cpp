#include <nds.h>
#include <stdio.h>
#include <stdlib.h>
#include <nds/interrupts.h>

#include "memtool.h"
#include "Emulator.h"
#include "filesys.h"
#include "strpcm.h"

static volatile bool strpcmPause;

volatile u32 VsyncPassedCount=0;
volatile bool strpcmRequestStop;
volatile bool strpcmRingEmptyFlag;
volatile u32 strpcmRingBufReadIndex;
volatile u32 strpcmRingBufWriteIndex;

s16 *strpcmRingLBuf=NULL;
s16 *strpcmRingRBuf=NULL;

static void strpcmUpdate(void);
#define CACHE_LINE_SIZE (32)

#include "tcmstart.h"
void ins_DC_FlushRangeOverrun(void *v,u32 size)
{
  size&=~(CACHE_LINE_SIZE-1);
  size+=CACHE_LINE_SIZE;
  
  DC_FlushRange(v,size);
  DC_InvalidateRange(v,size);
}
#include "tcmend.h"

#include "tcmstart.h"
void InterruptHandler_IPC_SYNC(void)
{  
  switch(IPC3->IR){
    case IR_NULL: {
    } break;
    case IR_NextSoundData: {
      DC_FlushAll();
      
      strpcmUpdate();

      const u32 Samples=IPC3->strpcmSamples;
      const u32 Channels=IPC3->strpcmChannels;
      
      ins_DC_FlushRangeOverrun(IPC3->strpcmLBuf,Samples*2);
      if(Channels==2) ins_DC_FlushRangeOverrun(IPC3->strpcmRBuf,Samples*2);
      
      IPC3->strpcmWriteRequest=0;
    } break;
  }
  
  IPC3->IR=IR_NULL;
}
#include "tcmend.h"

void strpcmStart(bool FastStart,u32 SampleRate,u32 SamplePerBuf,u32 ChannelCount,EstrpcmFormat strpcmFormat)
{  
  DC_FlushAll();
  
  iprintf("strpcm: Stage 1\n");
  
  while(IPC3->strpcmControl!=strpcmControl_NOP){
    swiWaitForVBlank();
  }

  strpcmRequestStop=false;
  strpcmPause=false;
  
  u32 Samples=SamplePerBuf;
  u32 RingSamples=Samples*strpcmRingBufCount;
  
  strpcmRingEmptyFlag=false;
  strpcmRingBufReadIndex=0;

  if(FastStart==false){
    strpcmRingBufWriteIndex=strpcmRingBufCount-1;
    }else{
    strpcmRingBufWriteIndex=1;
  }
  
  strpcmRingLBuf=(s16*)safemalloc(RingSamples*2);
  strpcmRingRBuf=(s16*)safemalloc(RingSamples*2);
  MemSet16DMA3(0,strpcmRingLBuf,RingSamples*2);
  MemSet16DMA3(0,strpcmRingRBuf,RingSamples*2);
  
  IPC3->strpcmFreq=SampleRate;
  IPC3->strpcmSamples=Samples;
  IPC3->strpcmChannels=ChannelCount;
  IPC3->strpcmFormat=strpcmFormat;
  
  IPC3->strpcmLBuf=(s16*)safemalloc(Samples*2);
  IPC3->strpcmRBuf=(s16*)safemalloc(Samples*2);
  MemSet16DMA3(0,IPC3->strpcmLBuf,Samples*2);
  MemSet16DMA3(0,IPC3->strpcmRBuf,Samples*2);
   
  DC_FlushAll();
  IPC3->strpcmControl=strpcmControl_Play;
   
  while(IPC3->strpcmControl!=strpcmControl_NOP)
  {
    swiWaitForIRQ();
  }
}

void strpcmStop(void)
{
  strpcmRequestStop=true;
  
  DC_FlushAll();
  while(IPC3->strpcmControl!=strpcmControl_NOP){
    DC_FlushAll();
  }
  
  IPC3->strpcmControl=strpcmControl_Stop;
  
  DC_FlushAll();
  while(IPC3->strpcmControl!=strpcmControl_NOP){
    DC_FlushAll();
  }
  
  strpcmRequestStop=false;
  strpcmPause=false;
  
  strpcmRingEmptyFlag=false;
  strpcmRingBufReadIndex=0;
  strpcmRingBufWriteIndex=0;
  
  if(strpcmRingLBuf!=NULL){
    safefree((void*)strpcmRingLBuf); strpcmRingLBuf=NULL;
  }
  if(strpcmRingRBuf!=NULL){
    safefree((void*)strpcmRingRBuf); strpcmRingRBuf=NULL;
  }
  
  IPC3->strpcmFreq=0;
  IPC3->strpcmSamples=0;
  IPC3->strpcmChannels=0;
  
  if(IPC3->strpcmLBuf!=NULL){
    safefree(IPC3->strpcmLBuf); IPC3->strpcmLBuf=NULL;
  }
  if(IPC3->strpcmRBuf!=NULL){
    safefree(IPC3->strpcmRBuf); IPC3->strpcmRBuf=NULL;
  }
}

// ----------------------------------------------

static u32 DMAFIXSRC;

#include "tcmstart.h"
void ins_MemSet16DMA2(u16 v,void *dst,u32 len)
{
#ifdef notuseMemDMA2
  MemSet16CPU(v,dst,len);
  return;
#endif

  DMAFIXSRC=(vu32)v+((vu32)v<<16);
  
  ins_DC_FlushRangeOverrun(&DMAFIXSRC,4);
  ins_DC_FlushRangeOverrun(dst,len);
  
  u8 *_dst=(u8*)dst;
  DMA2_SRC = (uint32)&DMAFIXSRC;
  
  DMA2_DEST = (uint32)_dst;
  DMA2_CR = DMA_ENABLE | DMA_SRC_FIX | DMA_DST_INC | DMA_16_BIT | (len>>1);
  while(DMA2_CR & DMA_BUSY);
  return;
}
#include "tcmend.h"

#include "tcmstart.h"
void ins_MemCopy16DMA2(void *src,void *dst,u32 len)
{
#ifdef notuseMemDMA2
  MemCopy16CPU(src,dst,len);
  return;
#endif

  ins_DC_FlushRangeOverrun(src,len);
  ins_DC_FlushRangeOverrun(dst,len);
  
  u8 *_src=(u8*)src;
  u8 *_dst=(u8*)dst;
  
  DMA2_SRC = (uint32)_src;
  DMA2_DEST = (uint32)_dst;
  DMA2_CR = DMA_ENABLE | DMA_SRC_INC | DMA_DST_INC | DMA_16_BIT | (len>>1);
  while(DMA2_CR & DMA_BUSY);
  return;
}
#include "tcmend.h"

#include "tcmstart.h"
void ins_MemCopy32swi256bit(void *src,void *dst,u32 len)
{
  swiFastCopy(src,dst,COPY_MODE_COPY | (len/4));
}
#include "tcmend.h"

void strpcmUpdate(void)
{
  u32 Samples=IPC3->strpcmSamples;
  const u32 Channels=IPC3->strpcmChannels;
  
  s16 *ldst=IPC3->strpcmLBuf;
  s16 *rdst=IPC3->strpcmRBuf;
  
  if((ldst==NULL)||(rdst==NULL)) 
  {
	  iprintf("strpcmUpdate: ldst, rdst is null\n");
	  return;
  }
  
  if((strpcmRingLBuf==NULL)||(strpcmRingRBuf==NULL))
  {
    ins_MemSet16DMA2(0,ldst,Samples*2);
    if(Channels==2) ins_MemSet16DMA2(0,rdst,Samples*2);
    return;
  }
  
  bool IgnoreFlag=false;
  
  u32 CurIndex=(strpcmRingBufReadIndex+1) & strpcmRingBufBitMask;

  s16 *lsrc=&strpcmRingLBuf[Samples*CurIndex];
  s16 *rsrc=&strpcmRingRBuf[Samples*CurIndex];
  
  if(strpcmPause==true) 
	  IgnoreFlag=true;
  
  if(CurIndex==strpcmRingBufWriteIndex)
  {
    strpcmRingEmptyFlag=true;
    IgnoreFlag=true;
  }

  if(IgnoreFlag==true){
    ins_MemSet16DMA2(0,ldst,Samples*2);

    if(Channels==2) 
		ins_MemSet16DMA2(0,rdst,Samples*2);	
    return;
  }
  
  ins_MemCopy16DMA2(lsrc,ldst,Samples*2);
  if(Channels==2) 
	  ins_MemCopy16DMA2(rsrc,rdst,Samples*2);
  
  strpcmRingBufReadIndex=CurIndex;
}

void strpcmSetVolume16(int v)
{
  if(v<0) v=0;
  if(64<v) v=64;
  
  IPC3->strpcmVolume16=v;
}

void strpcmSetPause(bool v)
{
  strpcmPause=v;
}

bool strpcmGetPause(void)
{
  return(strpcmPause);
}

EstrpcmFormat GetOversamplingFactorFromSampleRate(u32 SampleRate)
{
  iprintf("SampleRate=%dHz\n",SampleRate);
  
  EstrpcmFormat SPF;
  
  if(SampleRate==(32768/4)){
    SPF=SPF_PCMx4;
    }else{
    if(SampleRate==(32768/2)){
      SPF=SPF_PCMx2;
      }else{
      if(SampleRate==(32768/1)){
        SPF=SPF_PCMx1;
        }else{
        if(SampleRate<=48000){
          SPF=SPF_PCMx4;
          }else{
          if(SampleRate<=96000){
            SPF=SPF_PCMx2;
            }else{
            SPF=SPF_PCMx1;
          }
        }
      }
    }
  }
  
  return(SPF);
}