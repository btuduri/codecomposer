
static volatile bool VsyncPassed;
static volatile bool irqVBlankBlock;
static int TPA_Count,TPA_X,TPA_Y;

static void InterruptHandler_VBlank(void)
{
  IPC3->heartbeat++;
  if(irqVBlankBlock==true) return;
  
  if (!(REG_KEYXY & 0x40)) {
    
    // I change this line from "_touchReadXY" to "touchReadXY" - KHS
	// touchPosition touchPos=_touchReadXY();
	touchPosition touchPos = touchReadXY();

    TPA_Count++;
    TPA_X+=touchPos.px;
    TPA_Y+=touchPos.py;
    }
    else
	{
    TPA_Count=0;
  }
  
  VsyncPassed=true;
}

//////////////////////////////////////////////////////////////////////

static inline void main_Proc_VsyncUpdate(void)
{
  uint16 but=0, xpx=0, ypx=0;//, batt=0, aux=0;
  int t1=0, t2=0;
  uint32 temp=0;
  uint8 ct[sizeof(IPC3->curtime)];
  
  // Read the X/Y buttons and the /PENIRQ line
  but = REG_KEYXY;
  
  {
    irqVBlankBlock=true;
    int c=TPA_Count,x=TPA_X,y=TPA_Y;
    irqVBlankBlock=false;
    
    TPA_Count=0;
    TPA_X=0;
    TPA_Y=0;
    
    if(c!=0){
      but&=~0x40;
      x=x/c;
      y=y/c;
      xpx=x;
      ypx=y;
      }else{
//      but|=0x40;
      if (!(but & 0x40)) {
        // touchPosition touchPos=_touchReadXY();
		touchPosition touchPos = touchReadXY();
        xpx=touchPos.px;
        ypx=touchPos.py;
      }
    }
  }
  
  
  // Read the time
  if(IPC3->curtimeFlag==true){
    rtcGetTime((uint8 *)ct);
    BCDToInteger((uint8 *)&(ct[1]), 7);
    
    u32 i;
    for(i=0; i<sizeof(ct); i++) {
      IPC3->curtime[i] = ct[i];
    }
    
    IPC3->curtimeFlag=false;
  }
  
  // Read the temperature
  // I replaced _touch... as well - kHS
  // temp = _touchReadTemperature(&t1, &t2);
  temp = touchReadTemperature(&t1, &t2);
  
  // Update the IPC struct
  IPC3->buttons   = but;
  IPC3->touchXpx  = xpx;
  IPC3->touchYpx  = ypx;
//  IPC3->battery   = batt;
//  IPC3->aux       = aux;

  IPC3->temperature = temp;
//  IPC3->tdiode1 = t1;
//  IPC3->tdiode2 = t2;
}

