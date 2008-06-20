#include <NDS.h>
#include "_console.h"
#include "_consoleWriteLog.h"

void _cwl(char *file,int line,u32 sp)
{
  char *seek=file;
  
  while(*seek!=0){
    if(*seek=='\\') file=seek;
    seek++;
  }
  
  _consolePrintf("%s%d[%x]\n",file,line,sp);
}

void PrfStart(void)
{
  TIMER0_CR=0;
  TIMER0_DATA=0;
  TIMER0_CR=TIMER_ENABLE | TIMER_DIV_64;
  TIMER1_CR=0;
  TIMER1_DATA=0;
  TIMER1_CR=TIMER_ENABLE | TIMER_CASCADE;
}

u32 PrfEnd(int data)
{
  u32 t0d=TIMER0_DATA;
  u32 t1d=TIMER1_DATA;
  
  u32 us=(t1d<<16) | t0d;
  
  us/=33;
  
  us*=64;
  
//  us=us*1965; // 1965.686=65536/33.34;
//  us=us*30; // 30.713=1024/33.34;
//  us=us*2; // 1.919=64/33.34;
  
  if(data!=-1){
    if(us<0){
      //_consolePrint(".");
      }else{
      //_consolePrintf("prf data=%d %6dus\n",data,us);
    }
  }
  
  return(us);
}