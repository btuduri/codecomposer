
static void InterruptHandler_Timer_Null(void)
{
}

#define MAX( x, y ) ( ( x > y ) ? x : y )
#define MIN( x, y ) ( ( x < y ) ? x : y )

__attribute__((noinline)) static void InterruptHandler_Timer1_SetSwapChannel(void)
{
  s16 *lbuf,*rbuf;
  
  if(strpcmCursorFlag==0){
    lbuf=strpcmL0;
    rbuf=strpcmR0;
    }else{
    lbuf=strpcmL1;
    rbuf=strpcmR1;
  }
  
  u32 channel=strpcmCursorFlag;
  
  // Left channel
  SCHANNEL_CR(channel) = 0;
  SCHANNEL_SOURCE(channel) = (uint32)lbuf;
  SCHANNEL_CR(channel) = SCHANNEL_ENABLE | SOUND_ONE_SHOT | SOUND_VOL(0x7F) | SOUND_PAN(0) | SOUND_16BIT;
  
  channel+=2;
  
  // Right channel
  SCHANNEL_CR(channel) = 0;
  SCHANNEL_SOURCE(channel) = (uint32)rbuf;
  SCHANNEL_CR(channel) = SCHANNEL_ENABLE | SOUND_ONE_SHOT | SOUND_VOL(0x7F) | SOUND_PAN(0x7F) | SOUND_16BIT;
  
  strpcmCursorFlag=1-strpcmCursorFlag;
  
  static s32 lastvol=-1;
  s32 vol=(s32)IPC3->strpcmVolume16;
  
  if(lastvol!=vol){
    lastvol=vol;
    if(vol<16){
      SOUND_CR = SOUND_ENABLE | SOUND_VOL(vol*0x08);
      }else{
      SOUND_CR = SOUND_ENABLE | SOUND_VOL(0x7f);
    }
  }
}

__attribute__((noinline)) void InterruptHandler_Timer1_ApplyVolume(s16 *lbuf,s16 *rbuf,u32 count)
{
  s32 vol=(s32)IPC3->strpcmVolume16;
  if(vol<=16) return;
  
  if((lbuf==NULL)||(rbuf==NULL)) return;
  
  u32 cnt;
  for(cnt=count;cnt!=0;cnt--){
    s32 Sample;
    
    Sample=(s32)*lbuf;
    Sample=MAX(MIN(32767,(Sample*vol)/16),-32768);
    *lbuf++=(s16)Sample;
    
    Sample=(s32)*rbuf;
    Sample=MAX(MIN(32767,(Sample*vol)/16),-32768);
    *rbuf++=(s16)Sample;
  }
}

__attribute__((noinline)) static void InterruptHandler_Timer1_OverSampling(u32 Multiple,s16 *lbuf,s16 *rbuf,u32 Samples)
{
  static s16 slastl=0,slastr=0;
  
  switch(Multiple){
    case 1: {
    } break;
    case 2: {
      s16 *lsrc=&lbuf[Samples*1],*rsrc=&rbuf[Samples*1];
      s16 *ldst=lbuf,*rdst=rbuf;
      s16 lastl=slastl,lastr=slastr;
      u32 cnt;
      for(cnt=Samples;cnt!=0;cnt--){
        s16 sample;
        
        sample=*lsrc++;
        *ldst++=lastl;
        *ldst++=(lastl+sample)/2;
        lastl=sample;
        
        sample=*rsrc++;
        *rdst++=lastr;
        *rdst++=(lastr+sample)/2;
        lastr=sample;
      }
      slastl=lastl; slastr=lastr;
    } break;
    case 4: {
      s16 *lsrc=&lbuf[Samples*3],*rsrc=&rbuf[Samples*3];
      s16 *ldst=lbuf,*rdst=rbuf;
      s16 lastl=slastl,lastr=slastr;
      
      u32 cnt;
      for(cnt=Samples;cnt!=0;cnt--){
        s16 sample,half;
        
        sample=*lsrc++;
        half=(lastl+sample)/2;
        *ldst++=lastl;
        *ldst++=(lastl+half)/2;
        *ldst++=half;
        *ldst++=(half+sample)/2;
        lastl=sample;
        
        sample=*rsrc++;
        half=(lastr+sample)/2;
        *rdst++=lastr;
        *rdst++=(lastr+half)/2;
        *rdst++=half;
        *rdst++=(half+sample)/2;
        lastr=sample;
      }
      slastl=lastl; slastr=lastr;
    } break;
    default: {
    }
  }
}

static void InterruptHandler_Timer1_PCM(void)
{
  InterruptHandler_Timer1_SetSwapChannel();
  
  s16 *lbuf,*rbuf;
  
  if(strpcmCursorFlag==0){
    lbuf=strpcmL0;
    rbuf=strpcmR0;
    }else{
    lbuf=strpcmL1;
    rbuf=strpcmR1;
  }
  
  s16 *lsrc=strpcmLBuf;
  s16 *rsrc=strpcmRBuf;
  
  u32 Samples=strpcmSamples;
  
  u32 Multiple=0;
  
  switch(strpcmFormat){
    case SPF_PCMx1: Multiple=1; break;
    case SPF_PCMx2: Multiple=2; break;
    case SPF_PCMx4: Multiple=4; break;
    default: Multiple=0; break;
  }
  
  if(IPC3->strpcmWriteRequest!=0){
    MemSet16DMA3(0,lbuf,Samples*2);
    MemSet16DMA3(0,rbuf,Samples*2);
    }else{
    s16 *ldst,*rdst;
    
    ldst=&lbuf[Samples*(Multiple-1)]; rdst=&rbuf[Samples*(Multiple-1)];
    
    if(strpcmChannels==2){
      MemCopy16DMA3(lsrc,ldst,Samples*2);
      MemCopy16DMA3(rsrc,rdst,Samples*2);
      }else{
      MemCopy16DMA3(lsrc,ldst,Samples*2);
      MemCopy16DMA3(lsrc,rdst,Samples*2);
    }
    
    IPC3->IR=IR_NextSoundData;
    REG_IPC_SYNC|=IPC_SYNC_IRQ_REQUEST;
    IPC3->strpcmWriteRequest=1;
    
    InterruptHandler_Timer1_ApplyVolume(ldst,rdst,Samples);
    
    InterruptHandler_Timer1_OverSampling(Multiple,lbuf,rbuf,Samples);
  }
}

static void InterruptHandler_Timer1_MP2(void)
{
}

static void InterruptHandler_Timer1_OGG(void)
{
}