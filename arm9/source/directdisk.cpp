#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <NDS.h>

#include "_const.h"

#include "memtool.h"

#include "directdisk.h"

#include "gba_nds_fat.h"
#include "disc_io.h"
#include "mediatype.h"
#include "filesys.h"
#include "shell.h"

extern LPIO_INTERFACE active_interface;

typedef struct {
  char Filename[FilenameMaxLength];
  u32 Sector,SectorMirror;
} TFileInfo;

static TFileInfo FileInfo[EDDFile_MaxCount];

static EDDSaveType DDSaveType=EDDST_None;

void DD_Init(EDDSaveType st)
{
  for(u32 idx=0;idx<EDDFile_MaxCount;idx++){
    FileInfo[idx].Filename[0]=0;
    FileInfo[idx].Sector=0;
    FileInfo[idx].SectorMirror=0;
  }
  
  const char *pPath=FATShellPath;
  
  snprintf(FileInfo[EDDFile_resume].Filename,FilenameMaxLength,"%s/resume.sav",pPath);
  snprintf(FileInfo[EDDFile_bookmrk0].Filename,FilenameMaxLength,"%s/bookmrk0.sav",pPath);
  snprintf(FileInfo[EDDFile_bookmrk1].Filename,FilenameMaxLength,"%s/bookmrk1.sav",pPath);
  snprintf(FileInfo[EDDFile_bookmrk2].Filename,FilenameMaxLength,"%s/bookmrk2.sav",pPath);
  snprintf(FileInfo[EDDFile_bookmrk3].Filename,FilenameMaxLength,"%s/bookmrk3.sav",pPath);
  snprintf(FileInfo[EDDFile_setting].Filename,FilenameMaxLength,"%s/codecomposer.sav",pPath);
  
  DDSaveType=st;
  
  switch(DDSaveType){
    case EDDST_None: {
    } break;
    case EDDST_FAT: {
      if(DIMediaType==DIMT_NONE){
        DDSaveType=EDDST_None;
        }else{
        if(active_interface==NULL){
          DDSaveType=EDDST_None;
          }else{
          if((active_interface->ul_Features & FEATURE_MEDIUM_CANWRITE)==0){
            iprintf("\nnot support disk write function.\nAdapter = %s\n",DIMediaName);
            DDSaveType=EDDST_None;
          }
        }
      }
    } break;
  }
}

EDDSaveType DD_GetSaveType(void)
{
  return(DDSaveType);
}

bool DD_isExists(EDDFile DDFile)
{
  switch(DDSaveType){
    case EDDST_None: {
    } break;
    case EDDST_FAT: {
      TFileInfo *pfi=&FileInfo[DDFile];
      FAT_FILE *fp=FAT_fopen(pfi->Filename,"r");
      if(fp!=NULL){
        FAT_fclose(fp);
        return(true);
      }
      iprintf("not found %s.\n",pfi->Filename);
    } break;
  }
  
  return(false);
}

static bool DD_InitFile_FAT(TFileInfo *pfi)
{
  iprintf("\nInitialize file [%s]\n",pfi->Filename);
  
  FAT_FILE *fp=FAT_fopen(pfi->Filename,"r");
  if(fp==NULL){
    iprintf("\nfile not found. [%s]\n",pfi->Filename);
    return(false);
  }
  
  u32 clus=fp->curClus;
  u32 Sector=FAT_ClustToSect_extern(clus);
  iprintf("Cluster=0x%08x\n",clus);
  iprintf("Sector=0x%08x\n",Sector);
  
  if(Sector==0){
    iprintf("\nSector is null.\n");
    return(false);
  }
  
  FAT_fseek(fp,0,SEEK_END);
  u32 fsize=FAT_ftell(fp);
  FAT_fseek(fp,0,SEEK_SET);
  
  if(fsize!=DD_SectorSize){
    iprintf("\nfile size is not %dbyte.\n",DD_SectorSize);
    return(false);
  }
  
  u8 *pfbuf0=(u8*)safemalloc(DD_SectorSize);
  u8 *pfbuf1=(u8*)safemalloc(DD_SectorSize);
  
  {
    iprintf("Test: direct disk reading.");
    
    // read source
    active_interface->fn_ReadSectors(Sector,1,pfbuf0);
    FAT_fread(pfbuf1,1,DD_SectorSize,fp);
    
    // verify
    for(u32 idx=0;idx<DD_SectorSize;idx++){
      if(pfbuf0[idx]!=pfbuf1[idx]){
        iprintf(" failed.\nposition = (%d,0x%02x!=0x%02x)\n",idx,pfbuf0[idx],pfbuf1[idx]);
        return(false);
      }
    }
    iprintf("\n");
  }
  
  {
    iprintf("Test: direct disk writing.");
    
    // write dummy
    for(u32 idx=0;idx<DD_SectorSize;idx++){
      pfbuf0[idx]=(idx-126)&0xff;
    }
    active_interface->fn_WriteSectors(Sector,1,pfbuf0);
    
    // read dummy
//    for(vu32 idx=0;idx<0x10000;idx++);
    for(u32 idx=0;idx<DD_SectorSize;idx++){
      pfbuf0[idx]=0xa5;
    }
    active_interface->fn_ReadSectors(Sector,1,pfbuf0);
    
    // verify
    for(u32 idx=0;idx<DD_SectorSize;idx++){
      if(pfbuf0[idx]!=((idx-126)&0xff)){
        iprintf(" failed.\nposition = (%d,0x%02x!=0x%02x)\n",idx,pfbuf0[idx],(idx-126)&0xff);
        return(false);
      }
    }
    
    // restore
    active_interface->fn_WriteSectors(Sector,1,pfbuf1);
    
    iprintf("\n");
  }
  
  safefree(pfbuf0);
  safefree(pfbuf1);
  
  FAT_fclose(fp);
  
  pfi->Sector=Sector;
  pfi->SectorMirror=Sector;
  
  return(true);
}

bool DD_isEnabled(void)
{
  bool res=false;
  
  switch(DDSaveType){
    case EDDST_None: res=false; break;
    case EDDST_FAT: res=true; break;
  }
  
  return(res);
}

bool DD_InitFile(EDDFile DDFile)
{
  vu16 ime=REG_IME;
  REG_IME=0;
  
  TFileInfo *pfi=&FileInfo[DDFile];
  
  pfi->Sector=0;
  pfi->SectorMirror=0;
  
  bool res=false;
  
  switch(DDSaveType){
    case EDDST_None: res=false; break;
    case EDDST_FAT: res=DD_InitFile_FAT(pfi); break;
  }
  
  REG_IME=ime;
  
  return(res);
}

// --------------------

static void fatinfochk(EDDFile DDFile)
{
  TFileInfo *pfi=&FileInfo[DDFile];
  
  if((pfi->Sector==0)||(pfi->Sector!=pfi->SectorMirror)){
    iprintf("Sector(%d)!=SectorMirror(%d)\n",pfi->Sector,pfi->SectorMirror);
    iprintf("Critical error!! CPU HALT for safety.\n");
    
    while(1);
  }
}

static inline void IPCSYNC_Enabled(void)
{
  REG_IE|=IRQ_IPC_SYNC;
}

static inline void IPCSYNC_Disabled(void)
{
  REG_IE&=~IRQ_IPC_SYNC;
}

static void FAT_ReadSector(EDDFile DDFile,void *pbuf,u32 bufsize)
{
  fatinfochk(DDFile);
  IPCSYNC_Disabled();
  DC_FlushRangeOverrun(pbuf,512);
  active_interface->fn_ReadSectors(FileInfo[DDFile].Sector,1,pbuf);
  IPCSYNC_Enabled();
}

static void FAT_WriteSector(EDDFile DDFile,void *pbuf,u32 bufsize)
{
  fatinfochk(DDFile);
  IPCSYNC_Disabled();
  DC_FlushRangeOverrun(pbuf,512);
  active_interface->fn_WriteSectors(FileInfo[DDFile].Sector,1,pbuf);
  IPCSYNC_Enabled();
}

// --------------------

void DD_ReadFile(EDDFile DDFile,void *pbuf,u32 bufsize)
{
  vu16 ime=REG_IME;
  REG_IME=0;
  
  if(((int)DDFile<0)||((int)EDDFile_MaxCount<=(int)DDFile)){
    iprintf("request error DDFile=%d\n",(int)DDFile);
    
    while(1);
  }
  
  if(pbuf==NULL){
    iprintf("DD_xSector(%d,0x%x,%d); pbuf=NULL.\n",DDFile,pbuf,bufsize);
    
    while(1);
  }
  
  if(bufsize!=DD_SectorSize){
    iprintf("DD_xSector(%d,0x%x,%d); Illigal bufsize error.\n",DDFile,pbuf,bufsize);
    
    while(1);
  }
  
  switch(DDSaveType){
    case EDDST_None: break;
    case EDDST_FAT: FAT_ReadSector(DDFile,pbuf,bufsize); break;
  }
  
  REG_IME=ime;
}

void DD_WriteFile(EDDFile DDFile,void *pbuf,u32 bufsize)
{
  vu16 ime=REG_IME;
  REG_IME=0;
  
  if(((int)DDFile<0)||((int)EDDFile_MaxCount<=(int)DDFile)){
    iprintf("request error DDFile=%d\n",(int)DDFile);
    
    while(1);
  }
  
  if(pbuf==NULL){
    iprintf("DD_xSector(%d,0x%x,%d); pbuf=NULL.\n",DDFile,pbuf,bufsize);
    
    while(1);
  }
  
  if(bufsize!=DD_SectorSize){
    iprintf("DD_xSector(%d,0x%x,%d); Illigal bufsize error.\n",DDFile,pbuf,bufsize);
    
    while(1);
  }
  
  switch(DDSaveType){
    case EDDST_None: break;
    case EDDST_FAT: FAT_WriteSector(DDFile,pbuf,bufsize); break;
  }
  
  REG_IME=ime;
}

