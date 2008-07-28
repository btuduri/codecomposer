#include <stdio.h>
#include <stdlib.h>

#include "nds.h"
#include "std.h"
#include "smidlib.h"
#include "memtool.h"
#include "smidlib_pch.h"
#include "filesys.h"

#include "powtbl_bin.h"
static u16 *powtbl=(u16*)powtbl_bin;
#define POWRANGE (1024)

#include "sintbl_bin.h"
static int *sintbl=(int*)sintbl_bin;
#define SINRANGE (120)

static TPCMStack PCMStack[PCMStackCount];
static TVariationMap VarMap_Tone;
static TVariationMap VarMap_Drum;
static ALIGNED_VAR_IN_DTCM TChannelInfo ChannelInfo[ChannelInfoCount];

static u32 PCHCount;
static TPCH1 ALIGNED_VAR_IN_DTCM PCH1[PCHCountMax];
static TPCH2 PCH2[PCHCountMax];
static u32 Note2FreqTableFix16[128];
static u32 SampleRate,SampleBufCount;
static u32 GenVolume;

static u32 PCMFileOffset;

bool MemoryOverflowFlag;

void PCH_SetProgramMap_Load(TProgramMap *pPrgMap, int FileHandle,u32 FileOffset)
{
  MemoryOverflowFlag=false;
  
  pPrgMap->FileHandle=FileHandle;
  pPrgMap->FileOffset=FileOffset;
  
  FileSys_fseek((pPrgMap->FileHandle),pPrgMap->FileOffset,SEEK_SET);
  
  FileSys_fread(pPrgMap->DataOffset,128,4,(pPrgMap->FileHandle));
  
  u32 idx=0;
  for(idx=0;idx<128;idx++){
    pPrgMap->ppData[idx]=NULL;
  }
}

void PCH_SetProgramMap(int FileHandle)
{
  iprintf("PCH_SetProgramMap: FileHandle is %d\n", FileHandle);

  int prg=0;
  MemSet32CPU(0,&VarMap_Tone,sizeof(TVariationMap));
  MemSet32CPU(0,&VarMap_Drum,sizeof(TVariationMap));
  for(prg=0;prg<128;prg++){
    VarMap_Tone.pPrgMaps[prg]=NULL;
    VarMap_Drum.pPrgMaps[prg]=NULL;
  }
  
  u32 ID;
  u32 ToneOffset[128];
  u32 DrumOffset[128];
  
  FileSys_fseek(FileHandle,0x0,SEEK_SET);
  FileSys_fread(&ID,1,4, FileHandle);
  
  for(prg=0;prg<128;prg++){
	FileSys_fread(&ToneOffset[prg],1,4,FileHandle);
  }

  for(prg=0;prg<128;prg++){
    FileSys_fread(&DrumOffset[prg],1,4,FileHandle);
  }
  
  FileSys_fread(&PCMFileOffset,1,4, FileHandle);
  
  iprintf("Load VarMap_Tone\n");
  for(prg=0;prg<128;prg++){
    if(ToneOffset[prg]!=0){
	  iprintf("ToneOffset[%d] is %d\n", prg, ToneOffset[prg]);
      VarMap_Tone.pPrgMaps[prg]=(TProgramMap*)safemalloc(sizeof(TProgramMap));
      PCH_SetProgramMap_Load(VarMap_Tone.pPrgMaps[prg],FileHandle,ToneOffset[prg]);
    }
  }
  
  for(prg=0;prg<128;prg++){
    if(DrumOffset[prg]!=0){
	  iprintf("DrumOffset[%d] is %d\n", prg, DrumOffset[prg]);
      VarMap_Drum.pPrgMaps[prg]=(TProgramMap*)safemalloc(sizeof(TProgramMap));
      PCH_SetProgramMap_Load(VarMap_Drum.pPrgMaps[prg],FileHandle,DrumOffset[prg]);
    }
  }
  
  u32 idx=0;

  for(idx = 0;idx<PCMStackCount;idx++){
    TPCMStack *pPCMStack=&PCMStack[idx];
    pPCMStack->PCMFileOffset=(u32)-1;
    pPCMStack->pData=NULL;
  }
}


static void PCH_FreeProgramMap_Free_Patch(TProgramPatch *pPrgPatch)
{
  if(pPrgPatch->pPrg==NULL) return;
  
  pPrgPatch->PatchCount=0;
  safefree(pPrgPatch->pPrg); pPrgPatch->pPrg=NULL;
}

static void PCH_FreeProgramMap_Free(TProgramMap *pPrgMap)
{
  u32 idx=0; 
  
  for(idx=0;idx<128;idx++){
    if(pPrgMap->ppData[idx]!=NULL){
      PCH_FreeProgramMap_Free_Patch(pPrgMap->ppData[idx]);
      safefree(pPrgMap->ppData[idx]); pPrgMap->ppData[idx]=NULL;
    }
  }
}

void PCH_FreeProgramMap(void)
{
  u32 idx=0;
  for(idx=0;idx<128;idx++){
    if(VarMap_Tone.pPrgMaps[idx]!=NULL){
      PCH_FreeProgramMap_Free(VarMap_Tone.pPrgMaps[idx]);
      safefree(VarMap_Tone.pPrgMaps[idx]); VarMap_Tone.pPrgMaps[idx]=NULL;
    }
    if(VarMap_Drum.pPrgMaps[idx]!=NULL){
      PCH_FreeProgramMap_Free(VarMap_Drum.pPrgMaps[idx]);
      safefree(VarMap_Drum.pPrgMaps[idx]); VarMap_Drum.pPrgMaps[idx]=NULL;
    }
  }
  
  for(idx=0;idx<PCMStackCount;idx++){
    TPCMStack *pPCMStack=&PCMStack[idx];
    pPCMStack->PCMFileOffset=(u32)-1;
    if(pPCMStack->pData!=NULL){
      safefree(pPCMStack->pData); pPCMStack->pData=NULL;
    }
  }
}

static void* PCMStack_GetEqual(u32 ofs)
{
  u32 idx=0;
  for(idx=0;idx<PCMStackCount;idx++){
    TPCMStack *pPCMStack=&PCMStack[idx];
    if(pPCMStack->PCMFileOffset==ofs){
      return(pPCMStack->pData);
    }
  }
  
  return(NULL);
}

static void PCMStack_Regist(u32 ofs,void *p)
{
  u32 idx=0;
  for(idx=0;idx<PCMStackCount;idx++){
    TPCMStack *pPCMStack=&PCMStack[idx];
    if(pPCMStack->PCMFileOffset==(u32)-1){
      pPCMStack->PCMFileOffset=ofs;
      pPCMStack->pData=p;
      return;
    }
  }
}

static u32 GetVariationNumber(TVariationMap *pVarMap,u32 VarNum,u32 PrgNum,bool DrumMode)
{
  if(128<=VarNum) return((u32)-1);
  if(128<=PrgNum) return((u32)-1);
  
  for(;VarNum!=0;VarNum--){
    if(pVarMap->pPrgMaps[VarNum]!=NULL){
      TProgramMap *pPrgMap=pVarMap->pPrgMaps[VarNum];
      if(pPrgMap!=NULL){
        u32 ofs=pPrgMap->DataOffset[PrgNum];
        if(ofs!=0) return(VarNum);
      }
      break;
    }
    if(DrumMode==true) break;
  }
  
  VarNum=0;
  
  {
    TProgramMap *pPrgMap=pVarMap->pPrgMaps[VarNum];
    if(pPrgMap!=NULL){
      if(DrumMode==false){
        return(VarNum);
        }else{
        if(pPrgMap->DataOffset[PrgNum]!=0) return(VarNum);
      }
    }
  }
  
  return((u32)-1);
}

static TProgramPatch *GetProgramPatchPtr(TVariationMap *pVarMap,u32 VarNum,u32 PrgNum,bool DrumMode)
{
  iprintf("VarNum is %d\n", VarNum);
  iprintf("PrgNum is %d\n", PrgNum);

  if(128<=VarNum) 
  {
	  iprintf("GetProgramPatchPtr: VarNum is above 128\n");
	  return(NULL);
  }

  if(128<=PrgNum) 
  {
	  iprintf("GetProgramPatchPtr: PrgNum is above 128\n");
	  return(NULL);
  }

  for(;VarNum!=0;VarNum--){
    if(pVarMap->pPrgMaps[VarNum]!=NULL){
      TProgramMap *pPrgMap=pVarMap->pPrgMaps[VarNum];
      if(pPrgMap!=NULL){
        if(pPrgMap->ppData[PrgNum]!=NULL) return(pPrgMap->ppData[PrgNum]);
      }
      break;
    }
    if(DrumMode==true) break;
  }
  
  VarNum=0;
  
  {
    TProgramMap *pPrgMap=pVarMap->pPrgMaps[VarNum];
    if(pPrgMap!=NULL)
	{
	  iprintf("GetProgramPatchPtr: pPrgMap is not Null\n");
    
	  if(DrumMode==false)
	  {
        if(pPrgMap->ppData[PrgNum]!=NULL) 
			return(pPrgMap->ppData[PrgNum]);
        
		PrgNum&=~7; // find captital tone
        
		if(pPrgMap->ppData[PrgNum]!=NULL) 
			return(pPrgMap->ppData[PrgNum]);
      }
	  else
	  {
        if(pPrgMap->ppData[PrgNum]!=NULL) 
			return(pPrgMap->ppData[PrgNum]);
      }
    }
	else
	{
		iprintf("GetProgramPatchPtr: pPrgMap is Null\n");
	}
  }
  
  iprintf("GetProgramPatchPtr: Kind of Exception\n");
  return(NULL);
}

static TProgram *GetProgramFromPatchPtr(TProgramPatch *pPrgPatch,s32 Note,bool DrumMode)
{
//  return(pPrgPatch->pPrg[0]);
  
  if(pPrgPatch==NULL) return(NULL);
  if(128<=Note) return(NULL);
  
  if(DrumMode==true) return(&pPrgPatch->pPrg[0]);
  
  if(pPrgPatch->PatchCount==0) return(NULL);
  if(pPrgPatch->PatchCount==1) return(&pPrgPatch->pPrg[0]);
  
  u32 NoteFreq=Note2FreqTableFix16[Note];
  
  u32 NearPatchIndex=0x100;
  u32 NearFreq=0xffffffff;
  
  u32 patidx=0;

  for(patidx=0;patidx<pPrgPatch->PatchCount;patidx++){
    u32 RootFreq=pPrgPatch->pPrg[patidx].RootFreq;
    
    u32 sub;
    if(NoteFreq<RootFreq){
      sub=RootFreq-NoteFreq;
      }else{
      sub=NoteFreq-RootFreq;
    }
    
    if(sub<=NearFreq){
      NearFreq=sub;
      NearPatchIndex=patidx;
    }
    
//    iprintf("%d,%d,%d,%d,%d\n",patidx,NoteFreq>>16,NearFreq>>16,RootFreq>>16,NearPatchIndex);
  }
  
  if(NearPatchIndex==0x100){
    iprintf("find near root freq from patch list??\n");
    return(NULL);
  }
  
//  iprintf("near %4d,%4d,%d\n",NoteFreq>>16,NearFreq>>16,NearPatchIndex);
  
  return(&pPrgPatch->pPrg[NearPatchIndex]);
}

bool PCH_LoadProgram(s32 Note,u32 var,u32 prg,bool DrumMode)
{
  if(MemoryOverflowFlag==true) 
  {
	  iprintf("PCH_LoadProgram: MemoryOverflowFlag is true\n");
	  return(false);
  }	  
  
  if(Note==0) 
  {
	  iprintf("PCH_LoadProgram: Note = 0\n");
	  return(false);
  }
	  
  
  TVariationMap *pVarMap;
  u32 VarNum,PrgNum;
  
  if(DrumMode==true){
    pVarMap=&VarMap_Drum;
    VarNum=prg;
    PrgNum=Note;
    }else{
    pVarMap=&VarMap_Tone;
    VarNum=var;
    PrgNum=prg;
  }
  
  VarNum=GetVariationNumber(pVarMap,VarNum,PrgNum,DrumMode);
  if(VarNum==(u32)-1) 
  {
	  iprintf("PCH_LoadProgram: GetVariationNumber problem\n");
	  return(false);
  }

  TProgramMap *pPrgMap=pVarMap->pPrgMaps[VarNum];
  if(pPrgMap==NULL) 
  {
	  iprintf("PCH_LoadProgram: pPrgMap is null\n");
	  return(false);
  }

  if(pPrgMap->ppData[PrgNum]!=NULL) return(true); // already loaded.
  
  u32 ofs=pPrgMap->DataOffset[PrgNum];
  
  if(ofs==0) return(false);
  
  if(DrumMode==true){
    iprintf("LoadDrum:%d,%d->%d,%d\n",prg,Note,VarNum,PrgNum);
    }else{
    iprintf("LoadTone:%d,%d->%d,%d\n",var,prg,VarNum,PrgNum);
  }
  
  pPrgMap->ppData[PrgNum]=(TProgramPatch*)safemalloc(sizeof(TProgramPatch));
  if(pPrgMap->ppData[PrgNum]==NULL){
    MemoryOverflowFlag=true;
    iprintf("malloc(%d); out of memory!!\n",sizeof(TProgramPatch));
    return(false);
  }
  
  TProgramPatch *pPrgPatch=pPrgMap->ppData[PrgNum];
  
  u32 patcount;
  
  FileSys_fseek((pPrgMap->FileHandle),pPrgMap->FileOffset+ofs,SEEK_SET);
  
  FileSys_fread(&patcount,4,1, (pPrgMap->FileHandle));
  pPrgPatch->PatchCount=patcount;
  
  pPrgPatch->pPrg=(TProgram*)safemalloc(sizeof(TProgram)*patcount);
  if(pPrgPatch->pPrg==NULL){
    MemoryOverflowFlag=true;
    iprintf("PatchLoad malloc(%d); out of memory!!\n",sizeof(TProgram)*patcount);
    pPrgPatch->PatchCount=0;
    free(pPrgMap->ppData[PrgNum]); pPrgMap->ppData[PrgNum]=NULL;
    return(false);
  }
  
  u32 patidx=0;

  for(patidx=0;patidx<patcount;patidx++){
    pPrgPatch->pPrg[patidx].pData=NULL;
  }
  
  bool memoryoverflow=false;
  
  for(patidx=0;patidx<patcount;patidx++){
    TProgram *pPrg=&pPrgPatch->pPrg[patidx];
    FileSys_fread(pPrg,1,ProgramPatchHeaderSize, (pPrgMap->FileHandle));
    
    if(pPrg->s16Flag==1){
      pPrg->Length/=2;
      pPrg->LoopStart/=2;
      pPrg->LoopEnd/=2;
    }
    
    pPrg->VibSweep=pPrg->VibSweep/120;
      
	int idx=0;
    for(idx=0;idx<6;idx++){
      pPrg->EnvRate[idx]*=96;
    }
    
    s32 f=((s32)pPrg->FractionStart)-((s32)pPrg->FractionEnd);
    if(f<0){
      pPrg->LoopStart--;
      f+=0x10;
    }
    pPrg->FractionStart=((u32)f)<<12;
  }
  
  for(patidx=0;patidx<patcount;patidx++){
    TProgram *pPrg=&pPrgPatch->pPrg[patidx];
    
    {
      void *p=PCMStack_GetEqual(pPrg->PCMOffset);
      if(p!=NULL){
        pPrg->pData=p;
        continue;
      }
    }
    
    FileSys_fseek((pPrgMap->FileHandle),PCMFileOffset+pPrg->PCMOffset,SEEK_SET);
    
#define WAVEID (0x45564157)
#define TTACID (0x43415454)

    u32 ID;
    FileSys_fread(&ID,1,4,(pPrgMap->FileHandle));
    
    switch(ID){
      case WAVEID: {
        u32 datasize;
        FileSys_fread(&datasize,1,4, (pPrgMap->FileHandle));
        
        pPrg->pData=safemalloc(datasize);
        if(pPrg->pData==NULL){
          memoryoverflow=true;
          break;
        }
        
        iprintf("Read RAWPCM. %dbytes\n",datasize);
        FileSys_fread(pPrg->pData,1,datasize, (pPrgMap->FileHandle));
      } break;
      case TTACID: {
        u32 CodeBufSize;
        u8 *pCodeBuf;
        
        FileSys_fread(&CodeBufSize,1,4, (pPrgMap->FileHandle));
        pCodeBuf=(u8*)safemalloc(CodeBufSize);
        
        if(pCodeBuf==NULL){
          memoryoverflow=true;
          break;
        }
        
        u32 SourceSamplesCount;
        FileSys_fread(&SourceSamplesCount,1,4, (pPrgMap->FileHandle));
        
        FileSys_fread(pCodeBuf,1,CodeBufSize, (pPrgMap->FileHandle));
        
        if(pPrg->s16Flag==false){
          s8 *pSourceSamples=(s8*)safemalloc(SourceSamplesCount*1);
          if(pSourceSamples==NULL){
            if(pCodeBuf!=NULL){
              safefree(pCodeBuf); pCodeBuf=NULL;
            }
            memoryoverflow=true;
            break;
          }
          iprintf("DecodeTTAC8bit. %dbytes %dsamples\n",CodeBufSize,SourceSamplesCount);
          TTAC_Decode_8bit_asm(pCodeBuf,pSourceSamples,SourceSamplesCount);
//          TTAC_Decode_6bit_asm(pCodeBuf,pSourceSamples,SourceSamplesCount);
          pPrg->pData=pSourceSamples;
          }else{
          s16 *pSourceSamples=(s16*)safemalloc(SourceSamplesCount*2);
          if(pSourceSamples==NULL){
            if(pCodeBuf!=NULL){
              safefree(pCodeBuf); pCodeBuf=NULL;
            }
            memoryoverflow=true;
            break;
          }
          iprintf("DecodeTTAC16bit. %dbytes %dsamples\n",CodeBufSize,SourceSamplesCount);
          TTAC_Decode_16bit_asm(pCodeBuf,pSourceSamples,SourceSamplesCount);
//          TTAC_Decode_12bit_asm(pCodeBuf,pSourceSamples,SourceSamplesCount);
          pPrg->pData=pSourceSamples;
        }
        
        if(pCodeBuf!=NULL){
          safefree(pCodeBuf); pCodeBuf=NULL;
        }
      } break;
      default: {
        iprintf("Fatal error! Unknown WAVE format.\n");
//        	// This cannot be implemented us, by KHS
      } break;
    }
    
    if(memoryoverflow==true) break;
    
    PCMStack_Regist(pPrg->PCMOffset,pPrg->pData);
  }
  
  if(memoryoverflow==true){
    MemoryOverflowFlag=true;
    iprintf("PatchLoad out of memory!!\n");

    pPrgPatch->PatchCount=0;
    free(pPrgMap->ppData[PrgNum]); pPrgMap->ppData[PrgNum]=NULL;
    return(false);
  }
  
  return(true);
}

static void PCH_ChannelInfoInit(u32 ch)
{
  TChannelInfo *_pCI=&ChannelInfo[ch];
  
  _pCI->DrumMode=false;
  _pCI->Vol=0;
  _pCI->Exp=0;
  _pCI->Reverb=0;
  _pCI->Pedal=false;
}

static void PCH_ChInit(u32 ch)
{
  TPCH1 *_pPCH1=&PCH1[ch];
  TPCH2 *_pPCH2=&PCH2[ch];
  
  _pPCH1->Enabled=false;
  
  _pPCH1->ChannelNum=(u32)0;
  _pPCH1->pChannelInfo=&ChannelInfo[_pPCH1->ChannelNum];
  _pPCH2->Note=0;
  _pPCH2->Vel=0;
  
  _pPCH2->Panpot=0;
  
  _pPCH1->OnClock=0;
  _pPCH1->OffClock=0;
  
  _pPCH1->GT=0;
  
  _pPCH1->VibEnable=false;
  _pPCH2->VibSweepAdd=0;
  _pPCH1->VibSweepCur=0;
  _pPCH2->VibAdd=0;
  _pPCH1->VibCur=0;
  _pPCH2->VibDepth=0;
  _pPCH1->VibPhase=0;
  
  _pPCH1->ModEnable=false;
  _pPCH2->ModAdd=0;
  _pPCH1->ModCur=0;
  _pPCH2->ModDepth=0;
  _pPCH1->ModPhase=0;
  
  _pPCH1->EnvState=EES_None;
  _pPCH1->EnvSpeed=0;
  _pPCH1->EnvCurLevel=0;
  _pPCH1->EnvEndLevel=0;
  _pPCH1->EnvRelLevel=0;
  
  _pPCH1->pPrg=NULL;
  _pPCH2->PrgPos=0;
  _pPCH2->PrgMstVol=0;
  _pPCH2->PrgVol=0;
  
  _pPCH2->FreqAddFix16=0;
  _pPCH2->FreqCurFix16=0;
  
  _pPCH2->LastSampleData=0;
  _pPCH2->CurSampleData=0;
}

bool PCH_Init(u32 _SampleRate,u32 _SampleBufCount,u32 MaxChannelCount,u32 _GenVolume)
{
  SampleRate=_SampleRate;
  SampleBufCount=_SampleBufCount;
  GenVolume=_GenVolume;
  
  u32 ch=0;
  int idx=0;
  PCHCount=MaxChannelCount;
  if(PCHCountMax<PCHCount) PCHCount=PCHCountMax;
  
  for(ch=0;ch<ChannelInfoCount;ch++){
    PCH_ChannelInfoInit(ch);
  }
  
  for(ch=0;ch<PCHCount;ch++){
    PCH_ChInit(ch);
  }
  
  for(idx=0;idx<128;idx++){
    s32 Note=idx-1;
    
    Note-=68; // NoteA4
    
    u32 basefreq=440;
    
    while(Note<0){
      basefreq/=2;
      Note+=12;
    }
    
    while(12<=Note){
      basefreq*=2;
      Note-=12;
    }
    
    Note2FreqTableFix16[idx]=(u32)(basefreq*(0x10000+(u32)powtbl[(Note*POWRANGE)]));
  }
  
  return(true);
}

void PCH_Free(void)
{
  SampleRate=0;
}

void PCH_AllSoundOff(void)
{
  u32 ch=0;
  for(ch=0;ch<PCHCount;ch++){
    PCH_ChInit(ch);
  }
}

void PCH_AllNoteOff(u32 trk)
{
  u32 ch=0;
  for(ch=0;ch<PCHCount;ch++){
    TPCH1 *_pPCH1=&PCH1[ch];
    TPCH2 *_pPCH2=&PCH2[ch];
    if(_pPCH1->ChannelNum==trk) _pPCH1->Enabled=false;
  }
}

void PCH_NextClock(void)
{
  u32 ch=0;
  for(ch=0;ch<PCHCount;ch++){
    TPCH1 *_pPCH1=&PCH1[ch];
    TPCH2 *_pPCH2=&PCH2[ch];
    if(_pPCH1->Enabled==true){
      _pPCH1->OnClock++;
      if(_pPCH1->pChannelInfo->DrumMode==true){
        if((120*1.5)==_pPCH1->OnClock){
          if(_pPCH1->EnvState!=EES_Release5){
            _pPCH1->EnvRelLevel=_pPCH1->EnvCurLevel;
            _pPCH1->EnvState=EES_Release5;
            _pPCH1->EnvSpeed=-0x100;//_pPCH1->pPrg->EnvRate[5];
            _pPCH1->EnvEndLevel=0;
          }
        }
      }
      }else{
      if(_pPCH1->pChannelInfo->Pedal==true){
        }else{
        _pPCH1->OffClock++;
        if((120*2)==_pPCH1->OffClock) PCH_ChInit(ch); // exclusive stop when 2sec over.
      }
    }
  }
  
  for(ch=0;ch<PCHCount;ch++){
    TPCH1 *_pPCH1=&PCH1[ch];
    TPCH2 *_pPCH2=&PCH2[ch];
    if(_pPCH1->VibEnable==true){
      _pPCH1->VibCur+=_pPCH2->VibAdd;
      while((120*0x10000)<=_pPCH1->VibCur){
        _pPCH1->VibCur-=120*0x10000;
      }
      _pPCH1->VibPhase=sintbl[_pPCH1->VibCur>>16]*_pPCH2->VibDepth/0x10000;
      if(_pPCH1->VibSweepCur!=0x10000){
        _pPCH1->VibSweepCur+=_pPCH2->VibSweepAdd;
        if(_pPCH1->VibSweepCur<0x10000){
          s32 Factor=_pPCH1->VibSweepCur>>8;
          _pPCH1->VibPhase=(_pPCH1->VibPhase*Factor)/0x100;
          }else{
          _pPCH1->VibSweepCur=0x10000;
        }
      }
    }
  }
  
  for(ch=0;ch<PCHCount;ch++){
    TPCH1 *_pPCH1=&PCH1[ch];
    TPCH2 *_pPCH2=&PCH2[ch];
    if(_pPCH1->ModEnable==true){
      _pPCH1->ModCur+=_pPCH2->ModAdd;
      while((120*0x10000)<=_pPCH1->ModCur){
        _pPCH1->ModCur-=120*0x10000;
      }
      _pPCH1->ModPhase=sintbl[_pPCH1->ModCur>>16]*_pPCH2->ModDepth/0x10000;
    }
  }
  
  for(ch=0;ch<PCHCount;ch++){
    TPCH1 *_pPCH1=&PCH1[ch];
    TPCH2 *_pPCH2=&PCH2[ch];
    switch(_pPCH1->EnvState){
      case EES_None: {
      } break;
      case EES_Attack: {
        bool next=false;
//        iprintf("a%x\n",_pPCH1->EnvCurLevel);
        
        if(_pPCH1->EnvSpeed==0){
          next=true;
          }else{
          int nextlevel=(int)_pPCH1->EnvCurLevel+_pPCH1->EnvSpeed;
          
          if((_pPCH1->EnvSpeed<0)&&(nextlevel<(int)_pPCH1->EnvEndLevel)) next=true;
          if((0<_pPCH1->EnvSpeed)&&((int)_pPCH1->EnvEndLevel<nextlevel)) next=true;
          
          _pPCH1->EnvCurLevel=(u32)nextlevel;
        }
        if(next==true){
          _pPCH1->EnvState=EES_Decay;
          _pPCH1->EnvCurLevel=_pPCH1->EnvEndLevel;
          _pPCH1->EnvSpeed=_pPCH1->pPrg->EnvRate[1];
          _pPCH1->EnvEndLevel=_pPCH1->pPrg->EnvOfs[1];
          if(_pPCH1->EnvEndLevel<_pPCH1->EnvCurLevel) _pPCH1->EnvSpeed=-_pPCH1->EnvSpeed;
        }
        _pPCH2->PrgVol=_pPCH2->PrgMstVol*_pPCH1->EnvCurLevel/0x10000;
      } break;
      case EES_Decay: {
        bool next=false;
//        iprintf("d%x\n",_pPCH1->EnvCurLevel);
        
        if(_pPCH1->EnvSpeed==0){
          next=true;
          }else{
          int nextlevel=(int)_pPCH1->EnvCurLevel+_pPCH1->EnvSpeed;
          
          if((_pPCH1->EnvSpeed<0)&&(nextlevel<(int)_pPCH1->EnvEndLevel)) next=true;
          if((0<_pPCH1->EnvSpeed)&&((int)_pPCH1->EnvEndLevel<nextlevel)) next=true;
          
          _pPCH1->EnvCurLevel=(u32)nextlevel;
        }
        if(next==true){
          if(true){ // if(_pPCH1->DrumMode==false){
            _pPCH1->EnvState=EES_Sustain;
            _pPCH1->EnvCurLevel=_pPCH1->EnvEndLevel;
            _pPCH1->EnvSpeed=_pPCH1->pPrg->EnvRate[2];
            _pPCH1->EnvEndLevel=_pPCH1->pPrg->EnvOfs[2];
            if(_pPCH1->EnvEndLevel<_pPCH1->EnvCurLevel) _pPCH1->EnvSpeed=-_pPCH1->EnvSpeed;
            }else{
            _pPCH1->EnvRelLevel=_pPCH1->EnvCurLevel;
            _pPCH1->EnvState=EES_Release3;
            _pPCH1->EnvSpeed=-_pPCH1->pPrg->EnvRate[3];
            _pPCH1->EnvEndLevel=(_pPCH1->pPrg->EnvOfs[3]*_pPCH1->EnvRelLevel)>>16;
          }
        }
        _pPCH2->PrgVol=_pPCH2->PrgMstVol*_pPCH1->EnvCurLevel/0x10000;
      } break;
      case EES_Sustain: {
//        iprintf("s%x\n",_pPCH1->EnvCurLevel);
        bool next=false;
        
        if(_pPCH1->EnvSpeed==0){
          next=true;
          }else{
          int nextlevel=(int)_pPCH1->EnvCurLevel+_pPCH1->EnvSpeed;
          
          if((_pPCH1->EnvSpeed<0)&&(nextlevel<(int)_pPCH1->EnvEndLevel)) next=true;
          if((0<_pPCH1->EnvSpeed)&&((int)_pPCH1->EnvEndLevel<nextlevel)) next=true;
          
          _pPCH1->EnvCurLevel=(u32)nextlevel;
        }
        if(next==true){
          _pPCH1->EnvState=EES_None;
          _pPCH1->EnvCurLevel=_pPCH1->EnvEndLevel;
          _pPCH1->EnvSpeed=0;
          _pPCH1->EnvEndLevel=0;
        }
        _pPCH2->PrgVol=_pPCH2->PrgMstVol*_pPCH1->EnvCurLevel/0x10000;
      } break;
      case EES_Release3: {
        bool next=false;
//        iprintf("d%x\n",_pPCH1->EnvCurLevel);
        
        if(_pPCH1->EnvSpeed==0){
          next=true;
          }else{
          int nextlevel=(int)_pPCH1->EnvCurLevel+_pPCH1->EnvSpeed;
          
          if(nextlevel<(int)_pPCH1->EnvEndLevel) next=true;
          
          _pPCH1->EnvCurLevel=(u32)nextlevel;
        }
        if(next==true){
          _pPCH1->EnvState=EES_Release4;
          _pPCH1->EnvCurLevel=_pPCH1->EnvEndLevel;
          _pPCH1->EnvSpeed=-_pPCH1->pPrg->EnvRate[4];
          _pPCH1->EnvEndLevel=(_pPCH1->pPrg->EnvOfs[4]*_pPCH1->EnvRelLevel)>>16;
        }
        _pPCH2->PrgVol=_pPCH2->PrgMstVol*_pPCH1->EnvCurLevel/0x10000;
      } break;
      case EES_Release4: {
        bool next=false;
//        iprintf("d%x\n",_pPCH1->EnvCurLevel);
        
        if(_pPCH1->EnvSpeed==0){
          next=true;
          }else{
          int nextlevel=(int)_pPCH1->EnvCurLevel+_pPCH1->EnvSpeed;
          
          if(nextlevel<(int)_pPCH1->EnvEndLevel) next=true;
          
          _pPCH1->EnvCurLevel=(u32)nextlevel;
        }
        if(next==true){
          _pPCH1->EnvState=EES_Release5;
          _pPCH1->EnvCurLevel=_pPCH1->EnvEndLevel;
          _pPCH1->EnvSpeed=-_pPCH1->pPrg->EnvRate[5];
//          _pPCH1->EnvEndLevel=(_pPCH1->pPrg->EnvOfs[5]*_pPCH1->EnvRelLevel)>>16;
          _pPCH1->EnvEndLevel=0;
        }
        _pPCH2->PrgVol=_pPCH2->PrgMstVol*_pPCH1->EnvCurLevel/0x10000;
      } break;
      case EES_Release5: {
        bool next=false;
//        iprintf("r%x\n",_pPCH1->EnvCurLevel);
        
        if(_pPCH1->EnvSpeed==0){
          next=true;
          }else{
          int nextlevel=(int)_pPCH1->EnvCurLevel+_pPCH1->EnvSpeed;
          
          if(nextlevel<(int)_pPCH1->EnvEndLevel) next=true;
          
          _pPCH1->EnvCurLevel=(u32)nextlevel;
        }
        if(next==true){
          PCH_ChInit(ch);
          }else{
          _pPCH2->PrgVol=_pPCH2->PrgMstVol*_pPCH1->EnvCurLevel/0x10000;
        }
      } break;
    }
  }
  
}

void PCH_ChangeVolume(u32 trk,u32 v)
{
  if(ChannelInfoCount<=trk){
	  iprintf("PCH_ChangeVolume : Channel overflow. ch=%d\n",trk);
    return;
  }
  
  TChannelInfo *_pCI=&ChannelInfo[trk];
  _pCI->Vol=v;
  
  u32 ch=0;
  for(ch=0;ch<PCHCount;ch++){
    TPCH1 *_pPCH1=&PCH1[ch];
    TPCH2 *_pPCH2=&PCH2[ch];
    if((_pPCH1->Enabled==true)&&(_pPCH1->ChannelNum==trk)){
      u32 vol;
      
      vol=(_pCI->Vol*_pCI->Exp*_pPCH2->Vel*_pPCH1->pPrg->MasterVolume)/(128*128*128*128/128);
      vol=vol*GenVolume/128;
      _pPCH2->PrgMstVol=vol;
      if(_pPCH1->pPrg->EnvelopeFlag==false){
        _pPCH2->PrgVol=vol;
        }else{
        _pPCH2->PrgVol=vol*_pPCH1->EnvCurLevel/0x10000;
      }
    }
  }
}

void PCH_ChangeExpression(u32 trk,u32 e)
{
  if(ChannelInfoCount<=trk){
	  iprintf("PCH_ChangeExpression: Channel overflow. ch=%d",trk);
    return;
  }
  
  TChannelInfo *_pCI=&ChannelInfo[trk];
  _pCI->Exp=e;
  
  u32 ch=0;
  for(ch=0;ch<PCHCount;ch++){
    TPCH1 *_pPCH1=&PCH1[ch];
    TPCH2 *_pPCH2=&PCH2[ch];
    if((_pPCH1->Enabled==true)&&(_pPCH1->ChannelNum==trk)){
      u32 vol;
      
      vol=(_pCI->Vol*_pCI->Exp*_pPCH2->Vel*_pPCH1->pPrg->MasterVolume)/(128*128*128*128/128);
      vol=vol*GenVolume/128;
      _pPCH2->PrgMstVol=vol;
      if(_pPCH1->pPrg->EnvelopeFlag==false){
        _pPCH2->PrgVol=vol;
        }else{
        _pPCH2->PrgVol=vol*_pPCH1->EnvCurLevel/0x10000;
      }
    }
  }
}

static inline u32 FreqTransAddFix16(u32 basefreq,u32 powval,u32 SrcSampleRate,u32 RootFreq)
{
  // basefreq is unsigned 15bit
  // (0x10000+powval) is unsigned 17bit
  // SrcSampleRate is unsigned 16bit
  // RootFreq is unsigned fix16.16bit
  // SampleRate is unsigned 16bit
  
  float f=basefreq*(0x10000+powval);
  f=(f*(SrcSampleRate*0x10000))/RootFreq;
  f/=SampleRate;
  
  return((u32)f);
}

void PCH_ChangePitchBend(u32 trk,s32 Pitch)
{
  if(ChannelInfoCount<=trk){
	  iprintf("PCH_ChangePitchBend: Channel overflow. ch=%d",trk);
    return;
  }
  
//  TChannelInfo *_pCI=&ChannelInfo[trk];
  
  u32 ch=0;
  for(ch=0;ch<PCHCount;ch++){
    TPCH1 *_pPCH1=&PCH1[ch];
    TPCH2 *_pPCH2=&PCH2[ch];
    if((_pPCH1->Enabled==true)&&(_pPCH1->ChannelNum==trk)){
      TProgram *pPrg=_pPCH1->pPrg;
      s32 Note=_pPCH2->Note;
      
      s32 TunePitch=Pitch;
      
      Note-=1;
      
      Note-=68; // NoteA4
      
      while(TunePitch<0){
        Note--;
        TunePitch+=POWRANGE;
      }
      
      while(POWRANGE<=TunePitch){
        Note++;
        TunePitch-=POWRANGE;
      }
      
      u32 basefreq=440;
      
      while(Note<0){
        basefreq/=2;
        Note+=12;
      }
      
      while(12<=Note){
        basefreq*=2;
        Note-=12;
      }
      
      _pPCH2->FreqAddFix16=FreqTransAddFix16(basefreq,(u32)powtbl[(Note*POWRANGE)+TunePitch],pPrg->SampleRate,pPrg->RootFreq);
    }
  }
}

void PCH_ChangePanpot(u32 trk,u32 p)
{
  if(ChannelInfoCount<=trk){
	  iprintf("PCH_ChangePanpot: Channel overflow. ch=%d",trk);
    return;
  }
    
  u32 ch=0;

  for(ch=0;ch<PCHCount;ch++){
    TPCH1 *_pPCH1=&PCH1[ch];
    TPCH2 *_pPCH2=&PCH2[ch];
    if((_pPCH1->Enabled==true)&&(_pPCH1->ChannelNum==trk)){
      _pPCH2->Panpot=p;
    }
  }
}

void PCH_ChangeModLevel(u32 trk,u32 ModLevel)
{
  u32 ch=0;
  if(ModLevel==0){
    for(ch=0;ch<PCHCount;ch++){
      TPCH1 *_pPCH1=&PCH1[ch];
      TPCH2 *_pPCH2=&PCH2[ch];
      if((_pPCH1->Enabled==true)&&(_pPCH1->ChannelNum==trk)){
        _pPCH1->ModEnable=false;
      }
    }
    }else{
    for(ch=0;ch<PCHCount;ch++){
      TPCH1 *_pPCH1=&PCH1[ch];
      TPCH2 *_pPCH2=&PCH2[ch];
      if((_pPCH1->Enabled==true)&&(_pPCH1->ChannelNum==trk)){
        _pPCH1->ModEnable=true;
        _pPCH2->ModDepth=ModLevel*10; // (128*10)/0x10000 per
        _pPCH2->ModDepth=(int)(_pPCH2->FreqAddFix16*_pPCH2->ModDepth/0x10000);
      }
    }
  }
}

static int PCH_FindEqualChannel(u32 trk,u32 Note)
{
  u32 ch=0;
  for(ch=0;ch<PCHCount;ch++){
    TPCH1 *_pPCH1=&PCH1[ch];
    TPCH2 *_pPCH2=&PCH2[ch];
    if(_pPCH1->Enabled==true){
      if((_pPCH1->ChannelNum==trk)&&(_pPCH2->Note==Note)){
        return(ch);
      }
    }
  }
  
  return(-1);
}

static int PCH_FindDisableChannel(u32 PlayChOnly)
{
  u32 ch=0;
  for(ch=0;ch<PCHCountMax;ch++){
    TPCH1 *_pPCH1=&PCH1[ch];
    TPCH2 *_pPCH2=&PCH2[ch];
    if((PlayChOnly==false)||(_pPCH1->pPrg!=NULL)){
      if(_pPCH2->FreqAddFix16==0) return(ch);
    }
  }
  
  {
    bool FindedDisabledCh=false;
    for(ch=0;ch<PCHCountMax;ch++){
      TPCH1 *_pPCH1=&PCH1[ch];
      TPCH2 *_pPCH2=&PCH2[ch];
      if((PlayChOnly==false)||(_pPCH1->pPrg!=NULL)){
        if((_pPCH1->Enabled==false)&&(_pPCH1->pChannelInfo->DrumMode==true)){
          FindedDisabledCh=true;
        }
      }
    }
    
    if(FindedDisabledCh==true){
      u32 MaxClock=0;
      for(ch=0;ch<PCHCountMax;ch++){
        TPCH1 *_pPCH1=&PCH1[ch];
        TPCH2 *_pPCH2=&PCH2[ch];
        if((PlayChOnly==false)||(_pPCH1->pPrg!=NULL)){
          if((_pPCH1->Enabled==false)&&(_pPCH1->pChannelInfo->DrumMode==true)){
            if(MaxClock<_pPCH1->OffClock) MaxClock=_pPCH1->OffClock;
          }
        }
      }
      for(ch=0;ch<PCHCountMax;ch++){
        TPCH1 *_pPCH1=&PCH1[ch];
        TPCH2 *_pPCH2=&PCH2[ch];
        if((PlayChOnly==false)||(_pPCH1->pPrg!=NULL)){
          if((_pPCH1->Enabled==false)&&(_pPCH1->pChannelInfo->DrumMode==true)){
            if(MaxClock==_pPCH1->OffClock) return(ch);
          }
        }
      }
    }
  }
  
  {
    bool FindedDisabledCh=false;
    for(ch=0;ch<PCHCountMax;ch++){
      TPCH1 *_pPCH1=&PCH1[ch];
      TPCH2 *_pPCH2=&PCH2[ch];
      if((PlayChOnly==false)||(_pPCH1->pPrg!=NULL)){
        if((_pPCH1->Enabled==true)&&(_pPCH1->pChannelInfo->DrumMode==true)){
          FindedDisabledCh=true;
        }
      }
    }
    
    if(FindedDisabledCh==true){
      u32 MaxClock=0;
      for(ch=0;ch<PCHCountMax;ch++){
        TPCH1 *_pPCH1=&PCH1[ch];
        TPCH2 *_pPCH2=&PCH2[ch];
        if((PlayChOnly==false)||(_pPCH1->pPrg!=NULL)){
          if((_pPCH1->Enabled==true)&&(_pPCH1->pChannelInfo->DrumMode==true)){
            if(MaxClock<_pPCH1->OnClock) MaxClock=_pPCH1->OnClock;
          }
        }
      }
      for(ch=0;ch<PCHCountMax;ch++){
        TPCH1 *_pPCH1=&PCH1[ch];
        TPCH2 *_pPCH2=&PCH2[ch];
        if((PlayChOnly==false)||(_pPCH1->pPrg!=NULL)){
          if((_pPCH1->Enabled==true)&&(_pPCH1->pChannelInfo->DrumMode==true)){
            if(MaxClock==_pPCH1->OnClock) return(ch);
          }
        }
      }
    }
  }
  
  {
    bool FindedDisabledCh=false;
    for(ch=0;ch<PCHCountMax;ch++){
      TPCH1 *_pPCH1=&PCH1[ch];
      TPCH2 *_pPCH2=&PCH2[ch];
      if((PlayChOnly==false)||(_pPCH1->pPrg!=NULL)){
        if(_pPCH1->Enabled==false){
          FindedDisabledCh=true;
        }
      }
    }
    
    if(FindedDisabledCh==true){
      u32 MaxClock=0;
      for(ch=0;ch<PCHCountMax;ch++){
        TPCH1 *_pPCH1=&PCH1[ch];
        TPCH2 *_pPCH2=&PCH2[ch];
        if((PlayChOnly==false)||(_pPCH1->pPrg!=NULL)){
          if(_pPCH1->Enabled==false){
            if(MaxClock<_pPCH1->OffClock) MaxClock=_pPCH1->OffClock;
          }
        }
      }
      for(ch=0;ch<PCHCountMax;ch++){
        TPCH1 *_pPCH1=&PCH1[ch];
        TPCH2 *_pPCH2=&PCH2[ch];
        if((PlayChOnly==false)||(_pPCH1->pPrg!=NULL)){
          if(_pPCH1->Enabled==false){
            if(MaxClock==_pPCH1->OffClock) return(ch);
          }
        }
      }
    }
  }
  
  {
    u32 MaxClock=0;
    for(ch=0;ch<PCHCountMax;ch++){
      TPCH1 *_pPCH1=&PCH1[ch];
      TPCH2 *_pPCH2=&PCH2[ch];
      if((PlayChOnly==false)||(_pPCH1->pPrg!=NULL)){
        if(MaxClock<_pPCH1->OnClock) MaxClock=_pPCH1->OnClock;
      }
    }
    for(ch=0;ch<PCHCountMax;ch++){
      TPCH1 *_pPCH1=&PCH1[ch];
      TPCH2 *_pPCH2=&PCH2[ch];
      if((PlayChOnly==false)||(_pPCH1->pPrg!=NULL)){
        if(MaxClock==_pPCH1->OnClock) return(ch);
      }
    }
  }
  
  iprintf("error!!");
  return(-1);
}

void PCH_NoteOn(u32 trk,u32 GT,s32 Note,s32 Pitch,u32 Vol,u32 Exp,u32 Vel,u32 var,u32 prg,u32 panpot,u32 reverb,bool DrumMode,u32 ModLevel)
{
  if(ChannelInfoCount<=trk){
	  iprintf("PCH_NoteOn: Channel overflow. ch=%d\n",trk);
    return;
  }
  
  {
    int EqualCh=PCH_FindEqualChannel(trk,Note);
    if(EqualCh!=-1){
      TPCH1 *_pPCH1=&PCH1[EqualCh];
      TPCH2 *_pPCH2=&PCH2[EqualCh];
      _pPCH1->GT=GT;

	  iprintf("PCH_NoteOn: EqualCh is not -1\n");
      return;
    }
  }
  
  TVariationMap *pVarMap;
  u32 VarNum,PrgNum;
  
  if(DrumMode==true){
    pVarMap=&VarMap_Drum;
    VarNum=prg;
    PrgNum=Note;
    }else{
    pVarMap=&VarMap_Tone;
    VarNum=var;
    PrgNum=prg;
  }
  
  TProgram *pPrg;
  
  {
    TProgramPatch *pPrgPatch=GetProgramPatchPtr(pVarMap,VarNum,PrgNum,DrumMode);
    if(pPrgPatch==NULL) 
	{
	    iprintf("PCH_NoteOn: pPrgPatch is NULL\n");
		return;
	}
		
    pPrg=GetProgramFromPatchPtr(pPrgPatch,Note,DrumMode);
    if(pPrg==NULL) 
	{
	    iprintf("PCH_NoteOn: pPrg is NULL\n");
		return;
	}
  }
  
  int ch=PCH_FindDisableChannel(false);
  
  if(ch==-1){
    iprintf("PCHON %d,%d Error ChannelEmpty.\n",trk,Note);
    return;
  }
  
  PCH_ChInit(ch);
  
  TPCH1 *_pPCH1=&PCH1[ch];
  TPCH2 *_pPCH2=&PCH2[ch];
  
  TChannelInfo *_pCI=&ChannelInfo[trk];
  
  _pPCH1->ChannelNum=trk;
  _pPCH1->pChannelInfo=&ChannelInfo[_pPCH1->ChannelNum];
  
  _pPCH1->Enabled=true;
  _pPCH1->GT=GT;
  _pCI->DrumMode=DrumMode;
  _pPCH2->Note=Note;
  _pCI->Vol=Vol;
  _pCI->Exp=Exp;
  _pPCH2->Vel=Vel;
  _pCI->Reverb=reverb;
  _pPCH1->pPrg=pPrg;
  _pPCH2->PrgPos=1;
  
  if(_pPCH1->pPrg->s16Flag==false){
    s8 *pData=(s8*)pPrg->pData;
    _pPCH2->LastSampleData=pData[0]<<8;
    _pPCH2->CurSampleData=pData[1]<<8;
    }else{
    s16 *pData=(s16*)pPrg->pData;
    _pPCH2->LastSampleData=pData[0];
    _pPCH2->CurSampleData=pData[1];
  }
  
  pPrg->VibRatio=0;
  
  if((pPrg->VibRatio==0)||(pPrg->VibDepth==0)){
    _pPCH1->VibEnable=false;
    }else{
    _pPCH1->VibEnable=true;
    if(pPrg->VibSweep==0){
      _pPCH2->VibSweepAdd=0;
      _pPCH1->VibSweepCur=0x10000;
      }else{
      _pPCH2->VibSweepAdd=pPrg->VibSweep;
      _pPCH1->VibSweepCur=0;
    }
    _pPCH2->VibAdd=pPrg->VibRatio;
    _pPCH1->VibCur=0;
    _pPCH2->VibDepth=0;
    _pPCH1->VibPhase=0;
  }
  
  if(ModLevel==0){
    _pPCH1->ModEnable=false;
    }else{
    _pPCH1->ModEnable=true;
  }
  _pPCH2->ModAdd=0x10000*8; // 1/8sec
  _pPCH1->ModCur=0;
  _pPCH2->ModDepth=ModLevel*8; // (128*8)/0x10000 per
  _pPCH1->ModPhase=0;
  
  if(pPrg->EnvelopeFlag==false){
    _pPCH1->EnvState=EES_None;
    }else{
    _pPCH1->EnvState=EES_Attack;
    _pPCH1->EnvSpeed=pPrg->EnvRate[0];
    _pPCH1->EnvCurLevel=0;
    _pPCH1->EnvEndLevel=pPrg->EnvOfs[0];
  }
  
  {
    u32 vol;
    
    vol=(_pCI->Vol*_pCI->Exp*_pPCH2->Vel*_pPCH1->pPrg->MasterVolume)/(128*128*128*128/128);
    vol=vol*GenVolume/128;
    _pPCH2->PrgMstVol=vol;
    if(pPrg->EnvelopeFlag==false){
      _pPCH2->PrgVol=vol;
      }else{
      _pPCH2->PrgVol=vol*_pPCH1->EnvCurLevel/0x10000;
    }
  }
  
  if(DrumMode==false){
    _pPCH2->Panpot=panpot;
    }else{
    _pPCH2->Panpot=pPrg->MasterPanpot;
    _pPCH2->Note=pPrg->Note;
    Note=_pPCH2->Note;
  }
  
  if(_pPCH2->Note==0){
    _pPCH2->FreqAddFix16=(u32)((float)pPrg->SampleRate/SampleRate*0x10000);
    _pPCH2->FreqCurFix16=0;
    }else{
    Note-=1;
    
    Note-=68; // NoteA4
    
    while(Pitch<0){
      Note--;
      Pitch+=POWRANGE;
    }
    
    while(POWRANGE<=Pitch){
      Note++;
      Pitch-=POWRANGE;
    }
    
    u32 basefreq=440;
    
    while(Note<0){
      basefreq/=2;
      Note+=12;
    }
    
    while(12<=Note){
      basefreq*=2;
      Note-=12;
    }
    
    _pPCH2->FreqAddFix16=FreqTransAddFix16(basefreq,(u32)powtbl[(Note*POWRANGE)+Pitch],pPrg->SampleRate,pPrg->RootFreq);
    _pPCH2->FreqCurFix16=0;
    
    _pPCH2->Panpot=panpot;
  }
  
  if(_pPCH1->VibEnable==true){
    _pPCH2->VibDepth=(int)(_pPCH2->FreqAddFix16*pPrg->VibDepth/0x10000);
  }
  
  if(_pPCH1->ModEnable==true){
    _pPCH2->ModDepth=(int)(_pPCH2->FreqAddFix16*_pPCH2->ModDepth/0x10000);
  }
}

void PCH_NoteOff(u32 trk,u32 Note,bool DrumMode)
{
  if(DrumMode==true) return;
  
  if(ChannelInfoCount<=trk){
	  iprintf("PCH_NoteOff: Channel overflow. ch=%d",trk);
    return;
  }
  
  TChannelInfo *_pCI=&ChannelInfo[trk];
  u32 ch=0;

  for(ch=0;ch<PCHCount;ch++){
    TPCH1 *_pPCH1=&PCH1[ch];
    TPCH2 *_pPCH2=&PCH2[ch];
    if(_pPCH1->Enabled==true){
      if((_pPCH1->ChannelNum==trk)&&(_pPCH2->Note==Note)){
        _pPCH1->Enabled=false;
        if(_pCI->Pedal==false){
          if((_pPCH1->pPrg->EnvelopeFlag==false)||(_pPCH1->EnvCurLevel==0)){
            PCH_ChInit(ch);
            }else{
            if(_pPCH1->EnvState<EES_Release3){
              _pPCH1->EnvRelLevel=_pPCH1->EnvCurLevel;
              _pPCH1->EnvState=EES_Release3;
              _pPCH1->EnvSpeed=-_pPCH1->pPrg->EnvRate[3];
              _pPCH1->EnvEndLevel=(_pPCH1->pPrg->EnvOfs[3]*_pPCH1->EnvRelLevel)>>16;
            }
          }
        }
      }
    }
  }
}

void PCH_PedalOn(u32 trk)
{
  if(ChannelInfoCount<=trk){
	  iprintf("PCH_PedalOn: Channel overflow. ch=%d",trk);
    return;
  }
  
  TChannelInfo *_pCI=&ChannelInfo[trk];
  
  if(_pCI->Pedal==true) return;
  
  _pCI->Pedal=true;
}

void PCH_PedalOff(u32 trk)
{
  if(ChannelInfoCount<=trk){
	  iprintf("PedalOff: Channel overflow. ch=%d",trk);
    return;
  }
  
  TChannelInfo *_pCI=&ChannelInfo[trk];
  
  if(_pCI->Pedal==false) return;
  
  _pCI->Pedal=false;
  
  u32 ch=0;
  for(ch=0;ch<PCHCount;ch++){
    TPCH1 *_pPCH1=&PCH1[ch];
    TPCH2 *_pPCH2=&PCH2[ch];
    if((_pPCH1->Enabled==false)&&(_pPCH1->ChannelNum==trk)){
      if((_pPCH1->pPrg->EnvelopeFlag==false)||(_pPCH1->EnvCurLevel==0)){
        PCH_ChInit(ch);
        }else{
        if(_pPCH1->EnvState<EES_Release3){
          _pPCH1->EnvRelLevel=_pPCH1->EnvCurLevel;
          _pPCH1->EnvState=EES_Release3;
          _pPCH1->EnvSpeed=-_pPCH1->pPrg->EnvRate[3];
          _pPCH1->EnvEndLevel=(_pPCH1->pPrg->EnvOfs[3]*_pPCH1->EnvRelLevel)>>16;
        }
      }
    }
  }
}

static __attribute__ ((noinline)) void PCH_Render_Loop(int ch,u32 FreqAddFix16,s32 *buf,int SampleCount)
{
  TPCH1 *_pPCH1=&PCH1[ch];
  TPCH2 *_pPCH2=&PCH2[ch];
  TProgram *pPrg=_pPCH1->pPrg;
  TChannelInfo *_pCI=_pPCH1->pChannelInfo;
  
  u32 PrgPos=_pPCH2->PrgPos;
  u32 FreqCurFix16=_pPCH2->FreqCurFix16;
  u32 FreqAddNormalFix16=FreqAddFix16;
  
  s32 LastSampleData=_pPCH2->LastSampleData;
  s32 CurSampleData=_pPCH2->CurSampleData;
  
  s32 lvol=(_pPCH2->PrgVol*_pPCH2->Panpot)<<2; // 7+7+2=16bit
  s32 rvol=(_pPCH2->PrgVol*(0x80-_pPCH2->Panpot))<<2; // 7+7+2=16bit
  
  u32 EndPoint;
  u32 LoopStart;
  s32 Fraction=pPrg->FractionStart;
  
  if(pPrg->LoopFlag==false){
    EndPoint=pPrg->Length;
    LoopStart=0x10000000;
    }else{
    EndPoint=pPrg->LoopEnd;
    LoopStart=pPrg->LoopStart;
  }


// in/out
#define REG_SampleCount "%0"
#define REG_PrgPos "%1"
#define REG_LastSampleData "%2"
#define REG_CurSampleData "%3"
#define REG_buf "%4"
#define REG_FreqCurFix16 "%5"

// in
#define REG_FreqAddFix16 "%6"
#define REG_lvol "%7"
#define REG_rvol "%8"
#define REG_0xffff "%9"

// in stack
#define REG_pData "%10"
#define REG_EndPoint "%11"
#define REG_LoopStart "%12"
#define REG_Fraction "%13"

// temp
#define REG_tmp0 "%10"
#define REG_tmp1 "%11"
#define REG_sl "%12"
#define REG_sr "%13"
  
  if(pPrg->s16Flag==false){
    s8 *pData=(s8*)pPrg->pData;
    vu32 reg0xffff=0xffff;
    
    asm volatile(
      "stmdb sp!,{"REG_pData","REG_EndPoint","REG_LoopStart","REG_Fraction"} \n"
      
      "add "REG_SampleCount",#1 \n"
      
      "PCH_Render_Loop_8bit_loop: \n"
      
      "subs "REG_SampleCount",#1 \n"
      "beq PCH_Render_Loop_8bit_end \n"
      
      "smulwb "REG_tmp0","REG_FreqCurFix16","REG_CurSampleData" \n"
      "sub "REG_tmp1","REG_0xffff","REG_FreqCurFix16" \n"
      "smlawb "REG_tmp0","REG_tmp1","REG_LastSampleData","REG_tmp0" \n"
      
      "ldmia "REG_buf",{"REG_sl","REG_sr"} \n"
      "smlawb "REG_sl","REG_lvol","REG_tmp0","REG_sl" \n"
      "smlawb "REG_sr","REG_rvol","REG_tmp0","REG_sr" \n"
      "stmia "REG_buf"!,{"REG_sl","REG_sr"} \n"
      
      "add "REG_FreqCurFix16","REG_FreqAddFix16" \n"
      
      "cmps "REG_FreqCurFix16",#0x10000 \n"
      "blo PCH_Render_Loop_8bit_loop \n"
      
      "ldmia sp,{"REG_pData","REG_EndPoint","REG_LoopStart","REG_Fraction"} \n"
      
      "PCH_Render_Loop_8bit_Looping: \n"
      
      "sub "REG_FreqCurFix16",#0x10000 \n"
      "add "REG_PrgPos",#1 \n"
      
      "cmps "REG_PrgPos","REG_EndPoint" \n"
      "beq PCH_Render_Loop_8bit_ProcEndPoint \n"
      
      "mov "REG_LastSampleData","REG_CurSampleData" \n"
      "ldrsb "REG_CurSampleData",["REG_pData","REG_PrgPos"] \n"
      
      "cmps "REG_FreqCurFix16",#0x10000 \n"
      "lsl "REG_CurSampleData",#8 \n"
      "blo PCH_Render_Loop_8bit_loop \n"
      "b PCH_Render_Loop_8bit_Looping \n"
      
      "PCH_Render_Loop_8bit_ProcEndPoint: \n"
      
      "cmps "REG_LoopStart",#0x10000000 \n"
      "beq PCH_Render_Loop_8bit_stop \n"
      
      "mov "REG_PrgPos","REG_LoopStart" \n"
      
      "add "REG_FreqCurFix16","REG_Fraction" \n"
      
      "mov "REG_LastSampleData","REG_CurSampleData" \n"
      "ldrsb "REG_CurSampleData",["REG_pData","REG_PrgPos"] \n"
      
      "cmps "REG_FreqCurFix16",#0x10000 \n"
      "lsl "REG_CurSampleData",#8 \n"
      "blo PCH_Render_Loop_8bit_loop \n"
      "b PCH_Render_Loop_8bit_Looping \n"
      
      "PCH_Render_Loop_8bit_stop: \n"
      
      "mov "REG_buf",#0 \n"
      
      "PCH_Render_Loop_8bit_end: \n"
      
      "ldmia sp!,{"REG_pData","REG_EndPoint","REG_LoopStart","REG_Fraction"} \n"
      
      : "+r"(SampleCount),"+r"(PrgPos),
        "+r"(LastSampleData),"+r"(CurSampleData),
        "+r"(buf),"+r"(FreqCurFix16)
      : "r"(FreqAddFix16),
        "r"(lvol),"r"(rvol),"r"(reg0xffff),
        "r"(pData),"r"(EndPoint),"r"(LoopStart),"r"(Fraction)
    );
    
    }else{
    s16 *pData=(s16*)pPrg->pData;
    vu32 reg0xffff=0xffff;
    
    asm volatile(
      "stmdb sp!,{"REG_pData","REG_EndPoint","REG_LoopStart","REG_Fraction"} \n"
      
      "add "REG_SampleCount",#1 \n"
      
      "PCH_Render_Loop_16bit_loop: \n"
      
      "subs "REG_SampleCount",#1 \n"
      "beq PCH_Render_Loop_16bit_end \n"
      
      "smulwb "REG_tmp0","REG_FreqCurFix16","REG_CurSampleData" \n"
      "sub "REG_tmp1","REG_0xffff","REG_FreqCurFix16" \n"
      "smlawb "REG_tmp0","REG_tmp1","REG_LastSampleData","REG_tmp0" \n"
      
      "ldmia "REG_buf",{"REG_sl","REG_sr"} \n"
      "smlawb "REG_sl","REG_lvol","REG_tmp0","REG_sl" \n"
      "smlawb "REG_sr","REG_rvol","REG_tmp0","REG_sr" \n"
      "stmia "REG_buf"!,{"REG_sl","REG_sr"} \n"
      
      "add "REG_FreqCurFix16","REG_FreqAddFix16" \n"
      
      "cmps "REG_FreqCurFix16",#0x10000 \n"
      "blo PCH_Render_Loop_16bit_loop \n"
      
      "ldmia sp,{"REG_pData","REG_EndPoint","REG_LoopStart","REG_Fraction"} \n"
      
      "PCH_Render_Loop_16bit_Looping: \n"
      
      "sub "REG_FreqCurFix16",#0x10000 \n"
      "add "REG_PrgPos",#1 \n"
      
      "cmps "REG_PrgPos","REG_EndPoint" \n"
      "beq PCH_Render_Loop_16bit_ProcEndPoint \n"
      
      "mov "REG_LastSampleData","REG_CurSampleData" \n"
      "mov "REG_CurSampleData","REG_PrgPos",lsl #1 \n"
      "ldrsh "REG_CurSampleData",["REG_pData","REG_CurSampleData"] \n"
      
      "cmps "REG_FreqCurFix16",#0x10000 \n"
      "blo PCH_Render_Loop_16bit_loop \n"
      "b PCH_Render_Loop_16bit_Looping \n"
      
      "PCH_Render_Loop_16bit_ProcEndPoint: \n"
      
      "cmps "REG_LoopStart",#0x10000000 \n"
      "beq PCH_Render_Loop_16bit_stop \n"
      
      "mov "REG_PrgPos","REG_LoopStart" \n"
      
      "add "REG_FreqCurFix16","REG_Fraction" \n"
      
      "mov "REG_LastSampleData","REG_CurSampleData" \n"
      "mov "REG_CurSampleData","REG_PrgPos",lsl #1 \n"
      "ldrsh "REG_CurSampleData",["REG_pData","REG_CurSampleData"] \n"
      
      "cmps "REG_FreqCurFix16",#0x10000 \n"
      "blo PCH_Render_Loop_16bit_loop \n"
      "b PCH_Render_Loop_16bit_Looping \n"
      
      "PCH_Render_Loop_16bit_stop: \n"
      
      "mov "REG_buf",#0 \n"
      
      "PCH_Render_Loop_16bit_end: \n"
      
      "ldmia sp!,{"REG_pData","REG_EndPoint","REG_LoopStart","REG_Fraction"} \n"
      
      : "+r"(SampleCount),"+r"(PrgPos),
        "+r"(LastSampleData),"+r"(CurSampleData),
        "+r"(buf),"+r"(FreqCurFix16)
      : "r"(FreqAddFix16),
        "r"(lvol),"r"(rvol),"r"(reg0xffff),
        "r"(pData),"r"(EndPoint),"r"(LoopStart),"r"(Fraction)
    );
  }

  if(buf==NULL) PCH_ChInit(ch);

#undef REG_SampleCount
#undef REG_PrgPos
#undef REG_LastSampleData
#undef REG_CurSampleData
#undef REG_buf
#undef REG_FreqCurFix16
#undef REG_0xffff
#undef REG_FreqAddFix16
#undef REG_lvol
#undef REG_rvol
#undef REG_pData
#undef REG_EndPoint
#undef REG_LoopStart
#undef REG_PrgFraction
#undef REG_tmp0
#undef REG_tmp1
#undef REG_sl
#undef REG_sr

  _pPCH2->LastSampleData=LastSampleData;
  _pPCH2->CurSampleData=CurSampleData;
  
  _pPCH2->FreqCurFix16=FreqCurFix16;
  _pPCH2->PrgPos=PrgPos;
}

static __attribute__ ((noinline)) void PCH_Render_Bulk(int ch,u32 FreqAddFix16,s32 *buf,u32 SampleCount)
{
  TPCH1 *_pPCH1=&PCH1[ch];
  TPCH2 *_pPCH2=&PCH2[ch];
  TProgram *pPrg=_pPCH1->pPrg;
  TChannelInfo *_pCI=_pPCH1->pChannelInfo;
  
  u32 PrgPos=_pPCH2->PrgPos;
  u32 FreqCurFix16=_pPCH2->FreqCurFix16;
  
  s32 LastSampleData=_pPCH2->LastSampleData;
  s32 CurSampleData=_pPCH2->CurSampleData;
  
  s32 lvol=(_pPCH2->PrgVol*_pPCH2->Panpot)<<2; // 7+7+2=16bit
  s32 rvol=(_pPCH2->PrgVol*(0x80-_pPCH2->Panpot))<<2; // 7+7+2=16bit
  
#define REG_SampleCount "%0"
#define REG_PrgPos "%1"
#define REG_LastSampleData "%2"
#define REG_CurSampleData "%3"
#define REG_buf "%4"
#define REG_FreqCurFix16 "%5"
#define REG_FreqAddFix16 "%6"
#define REG_pData "%7"
#define REG_lvol "%8"
#define REG_rvol "%9"

#define REG_0xffff "r0"
#define REG_tmp0 "r1"
#define REG_tmp1 "r2"
#define REG_sl "r3"
#define REG_sr REG_pData

  if(pPrg->s16Flag==false){
    s8 *pData=(s8*)pPrg->pData;
    asm volatile(
      "sub sp,#4 \n"
      "str "REG_pData",[sp,#0] \n"
      
      "add "REG_SampleCount",#1 \n"
      
      "mov "REG_0xffff",#0x10000 \n"
      "sub "REG_0xffff",#1 \n"
      
      "PCH_Render_Bulk_8bit_loop: \n"
      
      "subs "REG_SampleCount",#1 \n"
      "beq PCH_Render_Bulk_8bit_end \n"
      
      "smulwb "REG_tmp0","REG_FreqCurFix16","REG_CurSampleData" \n"
      "sub "REG_tmp1","REG_0xffff","REG_FreqCurFix16" \n"
      "smlawb "REG_tmp0","REG_tmp1","REG_LastSampleData","REG_tmp0" \n"
      
      "ldmia "REG_buf",{"REG_sl","REG_sr"} \n"
      "smlawb "REG_sl","REG_lvol","REG_tmp0","REG_sl" \n"
      "smlawb "REG_sr","REG_rvol","REG_tmp0","REG_sr" \n"
      "stmia "REG_buf"!,{"REG_sl","REG_sr"} \n"
      
      "add "REG_FreqCurFix16","REG_FreqAddFix16" \n"
      "cmps "REG_FreqCurFix16",#0x10000 \n"
      "blo PCH_Render_Bulk_8bit_loop \n"
      
      "ldr "REG_pData",[sp,#0] \n"
      
      "add "REG_PrgPos","REG_FreqCurFix16",lsr #16 \n"
      "and "REG_FreqCurFix16","REG_0xffff" \n"
      "sub "REG_tmp0","REG_PrgPos",#1 \n"
      "ldrsb "REG_LastSampleData",["REG_pData","REG_tmp0"] \n"
      "ldrsb "REG_CurSampleData",["REG_pData","REG_PrgPos"] \n"
      "lsl "REG_LastSampleData",#8 \n"
      "lsl "REG_CurSampleData",#8 \n"
      
      "b PCH_Render_Bulk_8bit_loop \n"
      
      "PCH_Render_Bulk_8bit_end: \n"
      
      "add sp,#4 \n"
      
      : "+r"(SampleCount),"+r"(PrgPos),
        "+r"(LastSampleData),"+r"(CurSampleData),
        "+r"(buf),"+r"(FreqCurFix16)
      : "r"(FreqAddFix16),
        "r"(pData),"r"(lvol),"r"(rvol)
      : REG_0xffff,REG_tmp0,REG_tmp1,REG_sl
    );
    
    }else{
    s16 *pData=(s16*)pPrg->pData;
    asm volatile(
      "sub sp,#4 \n"
      "str "REG_pData",[sp,#0] \n"
      
      "add "REG_SampleCount",#1 \n"
      
      "mov "REG_0xffff",#0x10000 \n"
      "sub "REG_0xffff",#1 \n"
      
      "PCH_Render_Bulk_16bit_loop: \n"
      
      "subs "REG_SampleCount",#1 \n"
      "beq PCH_Render_Bulk_16bit_end \n"
      
      "smulwb "REG_tmp0","REG_FreqCurFix16","REG_CurSampleData" \n"
      "sub "REG_tmp1","REG_0xffff","REG_FreqCurFix16" \n"
      "smlawb "REG_tmp0","REG_tmp1","REG_LastSampleData","REG_tmp0" \n"
      
      "ldmia "REG_buf",{"REG_sl","REG_sr"} \n"
      "smlawb "REG_sl","REG_lvol","REG_tmp0","REG_sl" \n"
      "smlawb "REG_sr","REG_rvol","REG_tmp0","REG_sr" \n"
      "stmia "REG_buf"!,{"REG_sl","REG_sr"} \n"
      
      "add "REG_FreqCurFix16","REG_FreqAddFix16" \n"
      "cmps "REG_FreqCurFix16",#0x10000 \n"
      "blo PCH_Render_Bulk_16bit_loop \n"
      
      "ldr "REG_pData",[sp,#0] \n"
      
      "add "REG_PrgPos","REG_FreqCurFix16",lsr #16 \n"
      "and "REG_FreqCurFix16","REG_0xffff" \n"
      "mov "REG_tmp0","REG_PrgPos",lsl #1 \n"
      "sub "REG_tmp1","REG_tmp0",#2 \n"
      "ldrsh "REG_LastSampleData",["REG_pData","REG_tmp1"] \n"
      "ldrsh "REG_CurSampleData",["REG_pData","REG_tmp0"] \n"
      
      "b PCH_Render_Bulk_16bit_loop \n"
      
      "PCH_Render_Bulk_16bit_end: \n"
      
      "add sp,#4 \n"
      
      : "+r"(SampleCount),"+r"(PrgPos),
        "+r"(LastSampleData),"+r"(CurSampleData),
        "+r"(buf),"+r"(FreqCurFix16)
      : "r"(FreqAddFix16),
        "r"(pData),"r"(lvol),"r"(rvol)
      : REG_0xffff,REG_tmp0,REG_tmp1,REG_sl
    );
  }
  
#undef REG_SampleCount
#undef REG_PrgPos
#undef REG_LastSampleData
#undef REG_CurSampleData
#undef REG_buf
#undef REG_FreqCurFix16
#undef REG_FreqAddFix16
#undef REG_pData
#undef REG_lvol
#undef REG_rvol

#undef REG_0xffff
#undef REG_tmp0
#undef REG_tmp1
#undef REG_sl
#undef REG_sr

  _pPCH2->LastSampleData=LastSampleData;
  _pPCH2->CurSampleData=CurSampleData;
  
  _pPCH2->FreqCurFix16=FreqCurFix16;
  _pPCH2->PrgPos=PrgPos;
}



// 이게 왜 False를 돌려줄까요?
bool PCH_RequestRender(u32 TagChannel)
{
  u32 ch=0;
  for(ch=0;ch<PCHCount;ch++){
    TPCH1 *_pPCH1=&PCH1[ch];
    TPCH2 *_pPCH2=&PCH2[ch];
    if(_pPCH1->ChannelNum==TagChannel){
	  // 아래 iprintf를 찍어보니 FreqAddFix16이 0입니다 ㅠ
      // iprintf("channel is %d\n", TagChannel);
	  // iprintf("FreqAddFix16 is %d\n", _pPCH2->FreqAddFix16);
      if((_pPCH1->pPrg!=NULL)&&(_pPCH2->FreqAddFix16!=0)){
        return(true);
      }
    }
  }
  
  // iprintf("PCH_RequestRender returns false\n");

  return(false);
}

void PCH_RenderStart(u32 SampleCount)
{
  u32 loopingchcount=0;
  u32 totalch=0;
  
  u32 ch=0;
  for(ch=0;ch<PCHCount;ch++){
    TPCH1 *_pPCH1=&PCH1[ch];
    TPCH2 *_pPCH2=&PCH2[ch];
    if((_pPCH1->pPrg!=NULL)&&(_pPCH2->FreqAddFix16!=0)){
      totalch++;
      
      TProgram *pPrg=_pPCH1->pPrg;
      
      u32 FreqAddFix16;
      
      {
        int tmp=(int)_pPCH2->FreqAddFix16;
        if(_pPCH1->VibEnable==true) tmp+=_pPCH1->VibPhase;
        if(_pPCH1->ModEnable==true) tmp+=_pPCH1->ModPhase;
        FreqAddFix16=(u32)tmp;
      }
      
      u32 EndPos=_pPCH2->PrgPos+((FreqAddFix16*SampleCount)>>16)+1;
      
      if(pPrg->LoopFlag==false){
        }else{
        if(pPrg->LoopEnd<=EndPos) loopingchcount++;
      }
    }
  }
  
  s32 limitch=PCHCountMax-(loopingchcount/1);
  if(limitch<1) limitch=1;
  
  while(limitch<=totalch){
    u32 fch=PCH_FindDisableChannel(true);
    if(fch==(u32)-1) break;
    PCH_ChInit(fch);
    
    totalch=0;
    for(ch=0;ch<PCHCount;ch++){
      TPCH1 *_pPCH1=&PCH1[ch];
      TPCH2 *_pPCH2=&PCH2[ch];
      if((_pPCH1->pPrg!=NULL)&&(_pPCH2->FreqAddFix16!=0)) totalch++;
    }
  }
}

void PCH_Render(u32 TagChannel,s32 *buf,u32 SampleCount)
{
  u32 ch=0;
  for(ch=0;ch<PCHCount;ch++){
    TPCH1 *_pPCH1=&PCH1[ch];
    TPCH2 *_pPCH2=&PCH2[ch];
    if(_pPCH1->ChannelNum==TagChannel){
      if((_pPCH1->pPrg!=NULL)&&(_pPCH2->FreqAddFix16!=0)){
        TProgram *pPrg=_pPCH1->pPrg;
        
        u32 FreqAddFix16;
        
        {
          int tmp=(int)_pPCH2->FreqAddFix16;
          if(_pPCH1->VibEnable==true) tmp+=_pPCH1->VibPhase;
          if(_pPCH1->ModEnable==true) tmp+=_pPCH1->ModPhase;
          FreqAddFix16=(u32)tmp;
        }
        
        u32 EndPos=_pPCH2->PrgPos+((FreqAddFix16*SampleCount)>>16)+1;
        
        bool BulkFlag=true;
        
        if(pPrg->LoopFlag==false){
          if(pPrg->Length<=EndPos) BulkFlag=false;
          }else{
          if(pPrg->LoopEnd<=EndPos) BulkFlag=false;
        }
        
        if(BulkFlag==true){
          PCH_Render_Bulk(ch,FreqAddFix16,buf,SampleCount);
          }else{
          PCH_Render_Loop(ch,FreqAddFix16,buf,SampleCount);
        }
      }
    }
  }
  
}

void PCH_RenderEnd(void)
{
}

u32 PCH_GetReverb(u32 TagChannel)
{
  return(ChannelInfo[TagChannel].Reverb);
}

int PCH_GT_GetNearClock(void)
{
  int NearClock=0x7fffffff;
  
  u32 ch=0;
  for(ch=0;ch<PCHCount;ch++){
    TPCH1 *_pPCH1=&PCH1[ch];
    TPCH2 *_pPCH2=&PCH2[ch];
    
    if(_pPCH1->Enabled==true){
      if(_pPCH1->GT!=0){
        if((s32)_pPCH1->GT<NearClock) NearClock=(s32)_pPCH1->GT;
      }
    }
  }
  
  if(NearClock==0x7fffffff) NearClock=0;
  
  return(NearClock);
}

void PCH_GT_DecClock(u32 clk)
{
  u32 ch=0;
  for(ch=0;ch<PCHCount;ch++){
    TPCH1 *_pPCH1=&PCH1[ch];
    TPCH2 *_pPCH2=&PCH2[ch];
    
    if(_pPCH1->Enabled==true){
      if(_pPCH1->GT!=0){
        _pPCH1->GT-=clk;
        if(_pPCH1->GT==0){
          PCH_NoteOff(_pPCH1->ChannelNum,_pPCH2->Note,_pPCH1->pChannelInfo->DrumMode);
        }
      }
    }
  }
}

bool PCH_isDrumMap(u32 TagChannel)
{
  return(ChannelInfo[TagChannel].DrumMode);
}

