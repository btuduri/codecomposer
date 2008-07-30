#include <stdio.h>
#include <stdlib.h>
#include <nds.h>
#include "_touch.h"

#include "../../ipc3.h"
#include "memtoolARM7.h"
#include "a7sleep.h"

#define abs(x)	((x)>=0?(x):-(x))

int vcount;
touchPosition first,tempPos;

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
  // REG_IME=0;
  
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
  
  // REG_IME=1;
}

static void strpcmStop()
{
  // REG_IME=0;
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
  // REG_IME=1;
}

static void VcountHandler() {
	static int lastbut = -1;
	
	uint16 but=0, x=0, y=0, xpx=0, ypx=0, z1=0, z2=0;

	but = REG_KEYXY;

	if (!( (but ^ lastbut) & (1<<6))) {
 
		tempPos = touchReadXY();

		x = tempPos.x;
		y = tempPos.y;
		xpx = tempPos.px;
		ypx = tempPos.py;
		z1 = tempPos.z1;
		z2 = tempPos.z2;
		
	} else {
		lastbut = but;
		but |= (1 <<6);
	}

	if ( vcount == 80 ) {
		first = tempPos;
	} else {
		if (	abs( xpx - first.px) > 10 || abs( ypx - first.py) > 10 ||
				(but & ( 1<<6)) ) {

			but |= (1 <<6);
			lastbut = but;

		} else {
			IPC->mailBusy = 1;
			IPC->touchX			= x;
			IPC->touchY			= y;
			IPC->touchXpx		= xpx;
			IPC->touchYpx		= ypx;
			IPC->touchZ1		= z1;
			IPC->touchZ2		= z2;
			IPC->mailBusy = 0;
		}
	}
	IPC->buttons		= but;
	vcount ^= (80 ^ 130);
	SetYtrigger(vcount);
}

static void VblankHandler()
{
	uint16 but=0, x=0, y=0, xpx=0, ypx=0, z1=0, z2=0, batt=0, aux=0;
	
	// Read the X/Y buttons and the /PENIRQ line
	
	but = REG_KEYXY;
	if (!(but & 0x40)) {
		// Read the touch screen
		touchPosition tempPos = touchReadXY();

		x = tempPos.x;
		y = tempPos.y;
		xpx = tempPos.px;
		ypx = tempPos.py;
	}
	
	// Update the IPC struct
	
	IPC->buttons   = but;
	IPC->touchX    = x;
	IPC->touchY    = y;
	IPC->touchXpx  = xpx;
	IPC->touchYpx  = ypx;
	IPC->touchZ1   = z1;
	IPC->touchZ2   = z2;
	IPC->battery   = batt;
	IPC->aux       = aux;
}

#include "main_irq_timer.h"
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
    
  /*
  VsyncPassed=false;
  bool LastCloseFlag=false;
  */

  // Keep the ARM7 out of main RAM
  while (1)
  {
	/*
	VblankHandler();

    if(VsyncPassed==false) 
		swiWaitForVBlank();
    
	VsyncPassed=false;
    

    if(IPC3->ReqVsyncUpdate!=0)
	{
      main_Proc_VsyncUpdate();
      if(IPC3->ReqVsyncUpdate==1) 
		  IPC3->ReqVsyncUpdate=0;
    }
    */

    if(IPC3->strpcmControl!=strpcmControl_NOP)
	{
      REG_IME=0;    
	  main_Proc_strpcmControl();
      REG_IME=1;
    }

	VblankHandler();
	swiWaitForVBlank();
    
	/*
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
	*/
  }
  return 0;
}