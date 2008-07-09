#include "smidlib.h"
#include "smidlib_pch.h"
#include "smidlib_mtrk.h"
#include "smidlib_sm.h"

#include <stdio.h>

static u8 *bdata;
static u32 bSampleRate,bSampleBufCount;
static u32 bMaxChannelCount;
static u32 bGenVolume;

void smidlibSetParam(u8 *data,u32 SampleRate,u32 SampleBufCount,u32 MaxChannelCount,u32 GenVolume)
{
  bdata=data;
  bSampleRate=SampleRate;
  bSampleBufCount=SampleBufCount;
  bMaxChannelCount=MaxChannelCount;
  bGenVolume=GenVolume;
}

bool smidlibStart(void)
{
  TStdMIDI *_StdMIDI=&StdMIDI;
  
  SM_Init();
  SM_LoadStdMIDI(bdata,bSampleRate);
  if(_StdMIDI->SM_Chank.Format==2){
    iprintf("not support Format2\n");
    return(false);
  }
  PCH_Init(bSampleRate,bSampleBufCount,bMaxChannelCount,bGenVolume);
  MTRKCC_Init();
  
  u32 TrackCount=_StdMIDI->SM_Chank.Track;
  u32 TrackNum=0;

  for(TrackNum=0;TrackNum<TrackCount;TrackNum++){
    TSM_Track *pSM_Track=&_StdMIDI->SM_Tracks[TrackNum];
    pSM_Track->WaitClock=SM_GetDeltaTime(pSM_Track);
  }
  
  return(true);
}

void smidlibFree(void)
{
  MTRKCC_Free();
  PCH_Free();
  SM_Free();
}

int smidlibGetNearClock(void)
{
  TStdMIDI *_StdMIDI=&StdMIDI;
  
  u32 TrackCount=_StdMIDI->SM_Chank.Track;
  
  int NearClock=0x7fffffff;
  
  u32 TrackNum=0;

  for(TrackNum=0;TrackNum<TrackCount;TrackNum++){
    TSM_Track *pSM_Track=&_StdMIDI->SM_Tracks[TrackNum];
    if(pSM_Track->EndFlag==false){
      if(pSM_Track->WaitClock<NearClock) NearClock=pSM_Track->WaitClock;
    }
  }
  
  if(NearClock==0x7fffffff) NearClock=0;
  
  return(NearClock);
}

bool smidlibNextClock(bool ShowEventMessage,bool EnableNote,int DecClock)
{
  TStdMIDI *_StdMIDI=&StdMIDI;
  
  u32 TrackCount=_StdMIDI->SM_Chank.Track;
  u32 TrackNum=0;

  while(1){
    if(SM_isAllTrackEOF()==true) return(false);
    
    for(TrackNum=0;TrackNum<TrackCount;TrackNum++){
      TSM_Track *pSM_Track=&_StdMIDI->SM_Tracks[TrackNum];
      if(pSM_Track->EndFlag==false){
        pSM_Track->WaitClock-=DecClock;
        while(pSM_Track->WaitClock==0){
          SM_ProcStdMIDI(ShowEventMessage,EnableNote,pSM_Track);
          
          if(pSM_Track->EndFlag==true){
            iprintf("end of Track\n");
            break;
          }
          
          pSM_Track->WaitClock+=SM_GetDeltaTime(pSM_Track);
        }
      }
    }
    if(_StdMIDI->FastNoteOn==true) break;
    DecClock=smidlibGetNearClock();
  }
  
  return(true);
}

void smidlibAllSoundOff(void)
{
  PCH_AllSoundOff();
}