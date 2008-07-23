
static inline void SoftPowerOff(void)
{
  // referrence from SaTa.'s document
  a7poff();
  while(1);
}

static inline void main_Proc_strpcmControl(void)
{
  switch(IPC3->strpcmControl){
    case strpcmControl_Play: {
      strpcmPlay();
      irqSet(IRQ_TIMER1, InterruptHandler_Timer_Null);
      switch(strpcmFormat){
        case SPF_PCMx1: 
		case SPF_PCMx2: 
		case SPF_PCMx4: 
			irqSet(IRQ_TIMER1, InterruptHandler_Timer1_PCM); 
			break;
      }
    } break;
    case strpcmControl_Stop: {
      strpcmStop();
      irqSet(IRQ_TIMER1, InterruptHandler_Timer_Null);
    } break;
    default: {
      strpcmStop();
      irqSet(IRQ_TIMER1, InterruptHandler_Timer_Null);
    } break;
  }
  IPC3->strpcmControl=strpcmControl_NOP;
}

static inline void main_Proc_LCDPowerControl(void)
{
  LastLCDPowerControl=IPC3->LCDPowerControl;
  IPC3->LCDPowerControl=LCDPC_NOP;
  LCDPowerApplyFlag=true;
}

static inline void main_Proc_Brightness(void)
{
  u8 data;
  
  data=PM_GetRegister(PM_NDSLITE_ADR);
  data&=~PM_NDSLITE_BRIGHTNESS_MASK;
  data|=PM_NDSLITE_BRIGHTNESS(IPC3->Brightness);
  
  PM_SetRegister(PM_NDSLITE_ADR,data);
  
  IPC3->Brightness=0xff;
}

static inline void main_Proc_LCDPowerApplyFlag(void)
{
//  a7lcd_select(PM_BACKLIGHT_BOTTOM | PM_BACKLIGHT_TOP); return;
  if(LastLCDPowerControl==LCDPC_SOFT_POWEROFF){ // exclusive
    SoftPowerOff();
    while(1);
  }
  if(REG_KEYXY == 0x00FF){ // PanelClose
    a7led(1);
    a7lcd_select(0);
    }else{
    switch(LastLCDPowerControl){
      case LCDPC_OFF_BOTH: {
        a7led(1);
        a7lcd_select(0);
      } break;
      case LCDPC_ON_BOTTOM: {
        a7led(0);
        a7lcd_select(PM_BACKLIGHT_BOTTOM);
      } break;
      case LCDPC_ON_TOP_LEDON: case LCDPC_ON_TOP_LEDBLINK: {
        a7led(0);
        a7lcd_select(PM_BACKLIGHT_TOP);
      } break;
      case LCDPC_ON_BOTH: {
        a7led(0);
        a7lcd_select(PM_BACKLIGHT_BOTTOM | PM_BACKLIGHT_TOP);
      } break;
      case LCDPC_SOFT_POWEROFF: {
        SoftPowerOff();
        while(1);
      }
      default: while(1); break; // this execute is bug.
    }
  }
}

static inline void main_Proc_Reset(ERESET RESET)
{
  switch(RESET){
    case RESET_NULL: return; break;
    case RESET_MainMemory: boot_GBAROM(0); break;
    case RESET_GBAMP: boot_GBAMP(); break;
    case RESET_GBAROM: boot_GBAROM(0x08000000); break;
  }
  
  while(1);
}