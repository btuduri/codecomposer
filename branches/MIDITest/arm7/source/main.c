//////////////////////////////////////////////////////////////////////
// Simple ARM7 stub (sends RTC, TSC, and X/Y data to the ARM 9)
// -- joat
// -- modified by Darkain and others
//////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <nds.h>
#include "_touch.h"

#include "../../ipc3.h"
#include "memtoolARM7.h"
#include "a7sleep.h"


#pragma Ospace
static u32 LastLCDPowerControl=LCDPC_ON_BOTH;
static bool LCDPowerApplyFlag=true;
static u32 strpcmCursorFlag=0;
static u32 strpcmSamples,strpcmChannels;
static EstrpcmFormat strpcmFormat;
static s16 *strpcmLBuf=NULL,*strpcmRBuf=NULL;
static s16 *strpcmL0=NULL,*strpcmL1=NULL,*strpcmR0=NULL,*strpcmR1=NULL;
#undef SOUND_FREQ

#define SOUND_FREQ(n)	(0x10000 - (16777216 / (n)))

static void strpcmPlay()
{
  REG_IME=0;
  
  // The following member does not exist anywhere in the project!
  // IPC3->dummy=0;
  IPC3->IR=IR_NULL;
  
  strpcmCursorFlag=0;
  
  strpcmFormat=IPC3->strpcmFormat;
  
  strpcmLBuf=IPC3->strpcmLBuf;
  strpcmRBuf=IPC3->strpcmRBuf;
  
  switch(strpcmFormat){
    case SPF_PCMx1: SetMemoryMode(false); break;
    case SPF_PCMx2: SetMemoryMode(false); break;
    case SPF_PCMx4: SetMemoryMode(false); break;
  }
  
  int Multiple=0;
  
  switch(strpcmFormat){
    case SPF_PCMx1: Multiple=1; break;
    case SPF_PCMx2: Multiple=2; break;
    case SPF_PCMx4: Multiple=4; break;
  }
    
  strpcmSamples=IPC3->strpcmSamples;
  strpcmChannels=IPC3->strpcmChannels;
  
  iprintf("strpcmSamples=%d\n",strpcmSamples);
  iprintf("strpcmChannels=%d\n",strpcmChannels);
  iprintf("strpcmFormat=%d\n",strpcmFormat);
  iprintf("Multiple=%d\n",Multiple);
  
  strpcmL0=(s16*)safemalloc(strpcmSamples*Multiple*2);
  strpcmL1=(s16*)safemalloc(strpcmSamples*Multiple*2);
  strpcmR0=(s16*)safemalloc(strpcmSamples*Multiple*2);
  strpcmR1=(s16*)safemalloc(strpcmSamples*Multiple*2);
  
  if((strpcmL0==NULL)||(strpcmL1==NULL)||(strpcmR0==NULL)||(strpcmR1==NULL)){
    a7led(3); while(1);
  }
  
  SCHANNEL_CR(0) = 0;
  SCHANNEL_CR(1) = 0;
  SCHANNEL_CR(2) = 0;
  SCHANNEL_CR(3) = 0;
  
  int f=IPC3->strpcmFreq*Multiple;
  
  TIMER0_DATA = SOUND_FREQ(f);
  TIMER0_CR = TIMER_DIV_1 | TIMER_ENABLE;
  TIMER1_DATA = 0x10000 - (strpcmSamples*Multiple*2);
  TIMER1_CR = TIMER_CASCADE | TIMER_IRQ_REQ | TIMER_ENABLE;
  
  u32 ch;

  for(ch=0;ch<4;ch++){
    SCHANNEL_CR(ch) = 0;
    SCHANNEL_TIMER(ch) = SOUND_FREQ(f);
    SCHANNEL_LENGTH(ch) = (strpcmSamples*Multiple*2) >> 2;
    SCHANNEL_REPEAT_POINT(ch) = 0;
  }
  
  IPC3->strpcmWriteRequest=0;
  
  REG_IME=1;
}

static void strpcmStop()
{
  TIMER0_CR = 0;
  TIMER1_CR = 0;
  
  u32 ch;
  for(ch=0;ch<4;ch++){
    SCHANNEL_CR(ch) = 0;
  }
  
  safefree(strpcmL0); strpcmL0=NULL;
  safefree(strpcmL1); strpcmL1=NULL;
  safefree(strpcmR0); strpcmR0=NULL;
  safefree(strpcmR1); strpcmR1=NULL;
  
  IPC3->IR=IR_NULL;
}

#pragma Otime
#include "main_irq_timer.h"
#pragma Ospace

#include "main_vsync.h"
#include "main_init.h"
#include "main_boot_gbamp.h"
#include "main_boot_gbarom.h"
#include "main_proc.h"

int main(void)
{
  iprintf("ARM7 Start\n");
  main_InitAll();
  
  while(IPC3->heartbeat==0);
    
  VsyncPassed=false;
  
  bool LastCloseFlag=false;
  
  // Keep the ARM7 out of main RAM
  while (1)
  {
    if(VsyncPassed==false) 
		swiWaitForVBlank();
    
	VsyncPassed=false;
    
    if(IPC3->ReqVsyncUpdate!=0)
	{
      main_Proc_VsyncUpdate();
      if(IPC3->ReqVsyncUpdate==1) 
		  IPC3->ReqVsyncUpdate=0;
    }
    
    if(IPC3->strpcmControl!=strpcmControl_NOP)
	{
      // IPC3->dummy=0;
      REG_IME=0;    
	  main_Proc_strpcmControl();
      REG_IME=1;
    }
    
    bool CurrentCloseFlag;
  
	if(REG_KEYXY==0x00FF)
	{
      CurrentCloseFlag=true;
    }
	else
	{
      CurrentCloseFlag=false;
    }
    
    if(LastCloseFlag!=CurrentCloseFlag)
	{
      if(CurrentCloseFlag==true)
	  {
        if(IPC3->WhenPanelClose==true)
		{
          IPC3->RequestShotDown=true;
        }
      }
      
      LastCloseFlag=CurrentCloseFlag;
      LCDPowerApplyFlag=true;
    }
    
    if(IPC3->LCDPowerControl!=LCDPC_NOP) 
		main_Proc_LCDPowerControl();
    
    if(IPC3->Brightness!=0xff) 
		main_Proc_Brightness();
    
    if(LCDPowerApplyFlag==true)
	{
      REG_IME=0;
      LCDPowerApplyFlag=false;
      main_Proc_LCDPowerApplyFlag();
      REG_IME=1;
    }
    
    if(IPC3->RESET!=RESET_NULL){
      main_Proc_Reset(IPC3->RESET);
      while(1);
    }
    
    if(IPC3->pExecPCM!=NULL)
	{
      TExecPCM *pep=IPC3->pExecPCM;
      SCHANNEL_CR(6)=0;
      swiWaitForVBlank();
      SCHANNEL_SOURCE(6)=pep->SAD;
      SCHANNEL_TIMER(6)=pep->TMR;
      SCHANNEL_REPEAT_POINT(6)=pep->PNT;
      SCHANNEL_LENGTH(6)=pep->LEN;
      SCHANNEL_CR(6)=pep->CNT;
      IPC3->pExecPCM=NULL;
    }
  }
  return 0;
}