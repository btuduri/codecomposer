
// read firmware.
// reference hbfirmware.zip/firmware/arm7/source/settings.c

//typedef void(*call0)(void);
typedef void(*call3)(u32,void*,u32);

//size must be a multiple of 4
static inline void read_nvram(u32 src, void *dst, u32 size) {
	((call3)0x2437)(src,dst,size);
}

//read firmware settings
static inline void load_PersonalData() {
	u32 src=0, count0=0, count1=0;

	read_nvram(0x20, &src, 4);		//find settings area
	src=(src&0xffff)*8;

	read_nvram(src+0x70, &count0, 4);	//pick recent copy
	read_nvram(src+0x170, &count1, 4);
	if((u16)count0<(u16)count1){
		src+=0x100;
	}
	
	read_nvram(src, PersonalData, 0x80);
	if(swiCRC16(0xffff,PersonalData,0x70) != ((u16*)PersonalData)[0x72/2]){ 	//invalid / corrupt?
		read_nvram(src^0x100, PersonalData, 0x80);	//try the older copy
	}
}

static inline void main_InitIRQ(void)
{
  REG_IME = 0;

  irqInit();
  irqEnable(IRQ_VBLANK);
	
  swiWaitForVBlank();

  SetYtrigger(80);
  vcount = 80;
	
  irqSet(IRQ_TIMER1, InterruptHandler_Timer_Null);
  irqSet(IRQ_VCOUNT, VcountHandler);
  irqSet(IRQ_VBLANK, InterruptHandler_VBlank);

  irqEnable(IRQ_VCOUNT);
  REG_IME = 1;
}

static inline void main_InitVsync(void)
{
  VsyncPassed=false;
  irqVBlankBlock=false;
  
  TPA_Count=0;
  TPA_X=0;
  TPA_Y=0;
}

#define PM_NDSLITE_ADR (4)
#define PM_NDSLITE_ISLITE BIT(6)
#define PM_NDSLITE_BRIGHTNESS(x) ((x & 0x03)<<0)
#define PM_NDSLITE_BRIGHTNESS_MASK (PM_NDSLITE_BRIGHTNESS(3))

static inline void main_InitNDSL(void)
{
  IPC3->isNDSLite = ( (PM_GetRegister(PM_NDSLITE_ADR) & PM_NDSLITE_ISLITE) != 0) ? true : false;
  if(IPC3->isNDSLite==false){
    IPC3->DefaultBrightness=0;
    }else{
    u8 data;
    data=PM_GetRegister(PM_NDSLITE_ADR);
    data&=PM_NDSLITE_BRIGHTNESS_MASK;
    IPC3->DefaultBrightness=data;
  }
  IPC3->Brightness=0xff;
}

static inline void main_InitSoundDevice(void)
{
  powerON(POWER_SOUND);
  SOUND_CR = SOUND_ENABLE | SOUND_VOL(0x7F);
  
  // POWER_CR&=~POWER_UNKNOWN; // wifi power off
  REG_POWERCNT &= ~POWER_UNKNOWN; // wifi power off

  swiChangeSoundBias(1,0x400);
  a7SetSoundAmplifier(true);
}

static inline void main_InitAll(void)
{
  REG_IME=0;
  REG_IE=0;
  
  // Clear DMA
  u32 i;
  for(i=0;i<0x30/4;i++){
    *((vu32*)(0x40000B0+i))=0;
  }
  
  IPC3->heartbeat=0;
  IPC3->UserLanguage=(u32)-1;
  
  IPC3->curtimeFlag=true;
  IPC3->strpcmControl=strpcmControl_NOP;
  IPC3->LCDPowerControl=LCDPC_ON_BOTH;
  IPC3->RequestShotDown=false;
  
  IPC3->RESET=RESET_NULL;
  IPC3->IR=IR_NULL;
  
  // Reset the clock if needed
  rtcReset();
  
  REG_SPICNT = SPI_ENABLE|SPI_CONTINUOUS|SPI_DEVICE_NVRAM;
  load_PersonalData();
  REG_SPICNT = 0;
  
  // How this code works?.....
  // IPC3->UserLanguage=PersonalData->language;
  
  _touchReadXY_AutoDetect();
  
  main_InitNDSL();
  
  main_InitVsync();
  
  main_InitIRQ();
  
  main_InitSoundDevice();
}
