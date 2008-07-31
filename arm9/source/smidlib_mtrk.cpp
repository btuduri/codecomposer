#include <nds.h>
#include <stdio.h>
#include "smidlib_pch.h"
#include "memtool.h"
#include "std.h"

typedef struct {
  u32 Program;
  s32 PitchBend;
  u32 DrumMap;
  s32 BendRange;
  
  u8 ccBankSelectMSB,ccBankSelectLSB,ccModulation,ccVolume,ccPanpot,ccExpression;
  bool ccSustainPedal;
  u8 ccReleaseTime,ccAttackTime,ccDecayTime,ccVibRate,ccVibDepth,ccVibDelay;
  u8 ccReverb,ccChorus,ccVariation;
  u8 ccMSB,ccLSB;
} TMTRK_Track;

#define MTRK_TrackCount (16)

typedef struct {
  TMTRK_Track Track[MTRK_TrackCount];
} TMTRK;

static ALIGNED_VAR_IN_DTCM TMTRK MTRK;

static ALIGNED_VAR_IN_DTCM TMTRK_Track DrumMap1,DrumMap2;

// -------------------------------------------------------

static void MTRKCC_InitTrack(TMTRK_Track *_Track,u32 trk)
{
  _Track->Program=0;
  _Track->PitchBend=0;
  
  switch(trk){
    case 9: _Track->DrumMap=1; break;
    case 0xfe: _Track->DrumMap=1; break;
    case 0xff: _Track->DrumMap=2; break;
    default: _Track->DrumMap=0; break;
  }
  
  _Track->BendRange=2;
  
  _Track->ccBankSelectMSB=0;
  _Track->ccBankSelectLSB=0;
  _Track->ccModulation=0;
  _Track->ccVolume=100;
  _Track->ccPanpot=64;
  _Track->ccExpression=127;
  _Track->ccSustainPedal=false;
  _Track->ccReleaseTime=0;
  _Track->ccAttackTime=0;
  _Track->ccDecayTime=0;
  _Track->ccVibRate=0;
  _Track->ccVibDepth=0;
  _Track->ccVibDelay=0;
  
  _Track->ccReverb=40;
  _Track->ccChorus=0;
  _Track->ccVariation=0;
  
  _Track->ccMSB=0xff;
  _Track->ccLSB=0xff;
}

void MTRKCC_Init(void)
{
  MemSet32CPU(0,&MTRK,sizeof(TMTRK));
  
  u32 trk=0;
  
  for(trk=0;trk<MTRK_TrackCount;trk++){
    MTRKCC_InitTrack(&MTRK.Track[trk],trk);
  }
  
  MemSet32CPU(0,&DrumMap1.Program,sizeof(TMTRK_Track));
  MTRKCC_InitTrack(&DrumMap1,0xfe);
  MemSet32CPU(0,&DrumMap2.Program,sizeof(TMTRK_Track));
  MTRKCC_InitTrack(&DrumMap2,0xff);
}

void MTRKCC_Free(void)
{

}

void MTRKCC_Proc(u32 trk,u8 data0,u8 data1)
{
  TMTRK *_MTRK=&MTRK;
  TMTRK_Track *_Track=&_MTRK->Track[trk];
  
  if(_Track->DrumMap!=0){
    if(_Track->DrumMap==1) _Track=&DrumMap1;
    if(_Track->DrumMap==2) _Track=&DrumMap2;
  }
  
  switch(data0){
    case 0: {
      _Track->ccBankSelectMSB=data1;
      iprintf("ccVariation(%x):%d\n",trk,_Track->ccBankSelectMSB);
    } break;
    case 32: _Track->ccBankSelectLSB=data1; break;
    case 1: {
      _Track->ccModulation=data1;
      PCH_ChangeModLevel(trk,_Track->ccModulation);
      iprintf("ccModulation(%x):%d\n",trk,_Track->ccModulation);
    } break;
    case 6: {
      if((_Track->ccMSB==0)&&(_Track->ccLSB==0)){
        if(data1<=24){
          _Track->BendRange=data1;
          iprintf("ccPitchBendRange(%x):%d\n",trk,_Track->BendRange);
        }
      }
      _Track->ccMSB=0xff;
      _Track->ccLSB=0xff;
    } break;
    case 7: {
      _Track->ccVolume=data1;
      PCH_ChangeVolume(trk,data1);
    } break;
    case 10: {
      _Track->ccPanpot=data1;
      PCH_ChangePanpot(trk,data1);
    } break;
    case 11: {
      _Track->ccExpression=data1;
      PCH_ChangeExpression(trk,data1);
    } break;
    case 64: {
      iprintf("ccSustainPedal(%x):%d\n",trk,data1);
      if(data1<64){
        _Track->ccSustainPedal=false;
        PCH_PedalOff(trk);
        }else{
        _Track->ccSustainPedal=true;
        PCH_PedalOn(trk);
      }
    } break;
    case 72: _Track->ccReleaseTime=data1; break;
    case 73: _Track->ccAttackTime=data1; break;
    case 75: _Track->ccDecayTime=data1; break;
    case 76: _Track->ccVibRate=data1; break;
    case 77: _Track->ccVibDepth=data1; break;
    case 78: _Track->ccVibDelay=data1; break;
    case 91: {
//      iprintf("ccReverb(%x):%d\n",trk,data1);
      _Track->ccReverb=data1;
    } break;
    case 93: _Track->ccChorus=data1; break;
    case 94: _Track->ccVariation=data1; break;
    case 100: _Track->ccLSB=data1; break;
    case 101: _Track->ccMSB=data1; break;
    case 120: PCH_AllSoundOff(); break; // All Sound off data1:Dummy 該当するすべての?イスを直ちに消音する。
    case 121: MTRKCC_InitTrack(&MTRK.Track[trk],trk); break; // Reset All Contorollers 0:Dummy 該当する?ャンネルのコントロ?ラ?を初期値にする。
    case 123: PCH_AllNoteOff(trk); break; // All Notes off 0:Dummy 関連するすべての?イスをノ?トオフする。
  }
}

void MTRK_SetProgram(u32 trk,u32 v)
{
  TMTRK *_MTRK=&MTRK;
  TMTRK_Track *_Track=&_MTRK->Track[trk];
  
  if(_Track->DrumMap!=0){
    if(_Track->DrumMap==1) _Track=&DrumMap1;
    if(_Track->DrumMap==2) _Track=&DrumMap2;
  }
  
  _Track->Program=v;
}

u32 MTRK_GetProgram(u32 trk)
{
  TMTRK *_MTRK=&MTRK;
  TMTRK_Track *_Track=&_MTRK->Track[trk];
  
  if(_Track->DrumMap!=0){
    if(_Track->DrumMap==1) _Track=&DrumMap1;
    if(_Track->DrumMap==2) _Track=&DrumMap2;
  }
  
  return _Track->Program;
}


void MTRK_ChangePitchBend(u32 trk,s32 p)
{
  TMTRK *_MTRK=&MTRK;
  TMTRK_Track *_Track=&_MTRK->Track[trk];
  
  if(_Track->DrumMap!=0){
    if(_Track->DrumMap==1) _Track=&DrumMap1;
    if(_Track->DrumMap==2) _Track=&DrumMap2;
  }
  
  _Track->PitchBend=p;
  
  s32 Pitch=_Track->PitchBend;
  
  if(_Track->BendRange==0){
    Pitch=0;
    }else{
    Pitch=Pitch*_Track->BendRange/(8192/1024);
  }
  
  PCH_ChangePitchBend(trk,Pitch);
}

void MTRK_NoteOn_LoadProgram(u32 trk,u32 Note,u32 program)
{
  TMTRK *_MTRK=&MTRK;
  TMTRK_Track *_Track=&_MTRK->Track[trk];
  
  if(_Track->DrumMap!=0){
    if(_Track->DrumMap==1) _Track=&DrumMap1;
    if(_Track->DrumMap==2) _Track=&DrumMap2;
  }
  
  if(_Track->DrumMap==0){
    if(PCH_LoadProgram(Note,_Track->ccBankSelectMSB,_Track->Program,false)==false){
      PCH_LoadProgram(Note,0,_Track->Program,false);
    }
    }else{
    PCH_LoadProgram(Note,_Track->ccBankSelectMSB,_Track->Program,true);
  }
}

void MTRK_NoteOn(u32 trk,u32 GT,u32 Note,u32 Vel)
{
  TMTRK *_MTRK=&MTRK;
  TMTRK_Track *_Track=&_MTRK->Track[trk];
  
  if(_Track->DrumMap!=0){
    if(_Track->DrumMap==1) _Track=&DrumMap1;
    if(_Track->DrumMap==2) _Track=&DrumMap2;
  }
  
//  iprintf("Map%d,%d,",trk,_Track->DrumMap);
  
  s32 Pitch=_Track->PitchBend;
  
  if(_Track->BendRange==0){
    Pitch=0;
    }else{
    Pitch=Pitch*_Track->BendRange/(8192/1024);
  }
  
  if(_Track->DrumMap==0){
    PCH_NoteOn(trk,GT,Note,Pitch,_Track->ccVolume,_Track->ccExpression,Vel,_Track->ccBankSelectMSB,_Track->Program,_Track->ccPanpot,_Track->ccReverb,false,_Track->ccModulation);
    }else{
    PCH_NoteOn(trk,GT,Note,Pitch,_Track->ccVolume,_Track->ccExpression,Vel,_Track->ccBankSelectMSB,_Track->Program,_Track->ccPanpot,_Track->ccReverb,true,_Track->ccModulation);
  }
}

void MTRK_NoteOff(u32 trk,u32 Note,u32 Vel)
{
  TMTRK *_MTRK=&MTRK;
  TMTRK_Track *_Track=&_MTRK->Track[trk];
  
  bool DrumMode=false;
  
  if(_Track->DrumMap!=0){
    DrumMode=true;
    if(_Track->DrumMap==1) _Track=&DrumMap1;
    if(_Track->DrumMap==2) _Track=&DrumMap2;
  }
  
  PCH_NoteOff(trk,Note,DrumMode);
}


void MTRK_SetExMap(u32 trk,u32 mode)
{
  TMTRK *_MTRK=&MTRK;
  TMTRK_Track *_Track=&_MTRK->Track[trk];
  
  _Track->DrumMap=mode;
  
  return;
  
  iprintf("sMap%d:%d,",trk,_Track->DrumMap);
  
  u32 idx=0;

  for(idx=0;idx<MTRK_TrackCount;idx++){
    iprintf("%d",_MTRK->Track[idx].DrumMap);
  }

  iprintf("\n");
  
  u32 key=~REG_KEYINPUT&0x3ff;
  while(key==0) key=~REG_KEYINPUT&0x3ff;
  while(key!=0) key=~REG_KEYINPUT&0x3ff;
}
