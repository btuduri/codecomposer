
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <nds.h>
#include <gbfs.h>

#include "_const.h"
#include "shell.h"
#include "memtool.h"
#include "filesys.h"
#include "gba_nds_fat.h"
#include "mediatype.h"
#include "cimfs.h"

#define FAT_FT_END (0)
#define FAT_FT_FILE (1)
#define FAT_FT_DIR (2)

char FATShellPath[PathnameMaxLength];
char FATShellSkinPath[PathnameMaxLength];
char FATShellPluginPath[PathnameMaxLength];
char FATShellCustomPath[PathnameMaxLength];
char FATShellCustomLangPath[PathnameMaxLength];

__attribute__((noinline)) bool Shell_FindShellPath_FindIns(char *pBasePath)
{
  char toppath[2]={0,0};
  if(pBasePath==NULL) pBasePath=toppath;
  
  iprintf("find.%s\n",pBasePath);
  
  {
    char fn[PathnameMaxLength];
    snprintf(fn,PathnameMaxLength,"%s/moonshl.ini",pBasePath);
    FAT_FILE *pf=FAT_fopen(fn,"r");
    if(pf!=NULL){
      FAT_fclose(pf);
      snprintf(FATShellPath,PathnameMaxLength,"%s",pBasePath);
      snprintf(FATShellSkinPath,PathnameMaxLength,"%s/skin",pBasePath);
      snprintf(FATShellPluginPath,PathnameMaxLength,"%s/plugin",pBasePath);
      snprintf(FATShellCustomPath,PathnameMaxLength,"%s/custom",pBasePath);
      return(true);
    }
  }
  
  if(FAT_CWD(pBasePath)==false) return(false);
  
  u32 PathCount=0;
  char *Paths[128];
  
#define FindPathMax (128)
  
  u32 idx=0;
  for(idx=0;idx<FindPathMax;idx++){
    Paths[idx]=(char*)safemalloc(PathnameMaxLength);
  }
  
  {
    u32 FAT_FileType;
    char fn[PathnameMaxLength];
    FAT_FileType=FAT_FindFirstFile(fn);
    
    while(FAT_FileType!=FAT_FT_END){ 
      switch(FAT_FileType){ 
        case FAT_FT_DIR: {
          if((strcmp(fn,".")!=0)&&(strcmp(fn,"..")!=0)){
            if(PathCount<FindPathMax){
              strncpy(Paths[PathCount],fn,PathnameMaxLength);
              PathCount++;
            }
          }
        } break;
        case FAT_FT_FILE: {
        } break;
      }
      
      FAT_FileType=FAT_FindNextFile(fn);
    }
  }
  
  bool res=false;
  
  for(idx = 0;idx<PathCount;idx++){
    char fn[PathnameMaxLength];
    snprintf(fn,PathnameMaxLength,"%s/%s",pBasePath,Paths[idx]);
    if(Shell_FindShellPath_FindIns(fn)==true){
      res=true;
      break;
    }
  }
  
  for(idx=0;idx<FindPathMax;idx++){
    safefree(Paths[idx]); Paths[idx]=NULL;
  }
  
#undef FindPathMax
  
  return(res);
}

bool Shell_FindShellPath(void)
{
  strcpy(FATShellPath,"/moonshl");
  strcpy(FATShellSkinPath,"/moonshl/skin");
  strcpy(FATShellPluginPath,"/moonshl/plugin");
  strcpy(FATShellCustomPath,"/moonshl/custom");
  FATShellCustomLangPath[0]=0;
  
  {
    if(FAT_CWD("/moonshl")==true){
      FAT_FILE *pf=FAT_fopen("/moonshl/moonshl.ini","r");
      if(pf!=NULL){
        FAT_fclose(pf);
        iprintf("found shell=%s\n",FATShellPath);
        return(true);
      }
    }
  }
  
  iprintf("find path...\n");
  
  if(Shell_FindShellPath_FindIns(NULL)==true){
    iprintf("found shell=%s\n",FATShellPath);
    return(true);
  }
  
  iprintf("can not found moonshl folder.\n");
  
  return(false);
}

void Shell_AutoDetect(void)
{
  if(FileSys_InitInterface()==true){
    iprintf("Detected adapter is [%s]\n",DIMediaName);
    return;
  }
  
  iprintf("Not found adapter.\n");
}

static void Shell_ReadFile_IMFS(const char *fn,void **pbuf,int *psize)
{
  s32 PathIndex=pIMFS->GetPathIndex("/moonshl");
  s32 FileIndex=pIMFS->GetIndexFromFilename(PathIndex,fn);
  
  if((PathIndex!=-1)&&(FileIndex!=-1)){
    *psize=pIMFS->GetFileDataSizeFromIndex(PathIndex,FileIndex);
    *pbuf=(void*)safemalloc(*psize);
    
    if(pIMFS->GetFileTypeFromIndex(PathIndex,FileIndex)==FT_FileFlat){ 
      u8 *data=pIMFS->GetFileDirectDataFromIndex(PathIndex,FileIndex);
      if(data!=NULL){
        MemCopy8CPU(data,*pbuf,*psize);
        return;
      }
      }else{ 
      pIMFS->GetFileDataFromIndex(PathIndex,FileIndex,(u8*)*pbuf);
      return;
    }
  }
  
  iprintf("skip //IMFS/moonshl/%s\n",fn);
  
  *psize=0;
  *pbuf=NULL;
}

void Shell_ReadFile(const char *fn,void **pbuf,int *psize)
{
  FAT_FILE *fh;
  
  {
    char fullfn[PathnameMaxLength];
    snprintf(fullfn,PathnameMaxLength,"%s/%s",FATShellPath,fn);
    fh=FAT_fopen(fullfn,"r");
  }
  
  if(fh!=NULL){
    FAT_fseek(fh,0,SEEK_END);
    *psize=FAT_ftell(fh);
    FAT_fseek(fh,0,SEEK_SET);
    *pbuf=(void*)safemalloc(*psize);
    
    FAT_fread(*pbuf,1,*psize,fh);
    FAT_fclose(fh);
    return;
  }
  
  Shell_ReadFile_IMFS(fn,pbuf,psize);
}

int Shell_OpenFile(const char *fn)
{
  FAT_FILE *fh;
  
  {
    char fullfn[PathnameMaxLength];
    snprintf(fullfn,PathnameMaxLength,"%s/%s",FATShellPath,fn);
    fh=FAT_fopen(fullfn,"r");
  }
  
  if(fh!=NULL){
    int ifh=FileSys_fopen_DirectMapping(fh);
    return(ifh);
  }
  
  return(0);
}

int Shell_SkinOpenFile(const char *fn)
{
  FAT_FILE *fh;
  
  {
    char fullfn[PathnameMaxLength];
    snprintf(fullfn,PathnameMaxLength,"%s/%s",FATShellSkinPath,fn);
    fh=FAT_fopen(fullfn,"r");
  }
  
  if(fh!=NULL){
    int ifh=FileSys_fopen_DirectMapping(fh);
    return(ifh);
  }
  
  return(0);
}

void Shell_ReadSkinFile(const char *fn,void **pbuf,int *psize)
{
  FAT_FILE *fh;
  
  {
    char fullfn[PathnameMaxLength];
    snprintf(fullfn,PathnameMaxLength,"%s/%s",FATShellSkinPath,fn);
    fh=FAT_fopen(fullfn,"r");
  }
  
  if(fh!=NULL){
    FAT_fseek(fh,0,SEEK_END);
    *psize=FAT_ftell(fh);
    FAT_fseek(fh,0,SEEK_SET);
    *pbuf=(void*)safemalloc(*psize);
    
    FAT_fread(*pbuf,1,*psize,fh);
    FAT_fclose(fh);
    return;
  }
  
  Shell_ReadFile(fn,pbuf,psize);
}

void Shell_ReadCustomFile(const char *fn,void **pbuf,int *psize)
{
  *psize=0;
  *pbuf=NULL;
  
  FAT_FILE *fh;
  
  {
    char fullfn[PathnameMaxLength];
    snprintf(fullfn,PathnameMaxLength,"%s/%s",FATShellCustomPath,fn);
    iprintf("load %s\n",fullfn);
    fh=FAT_fopen(fullfn,"r");
  }
  
  if(fh!=NULL){
    FAT_fseek(fh,0,SEEK_END);
    *psize=FAT_ftell(fh);
    FAT_fseek(fh,0,SEEK_SET);
    *pbuf=(void*)safemalloc(*psize);
    
    FAT_fread(*pbuf,1,*psize,fh);
    FAT_fclose(fh);
    return;
  }
}

void Shell_SetCustomLangID(const u32 UserLanguage)
{
  {
    char fn[PathnameMaxLength];
    u32 ul=UserLanguage;
    snprintf(fn,PathnameMaxLength,"%s/lang%d/_lang.ini",FATShellCustomPath,ul);
    FAT_FILE *fh=FAT_fopen(fn,"r");
    if(fh!=NULL){
      FAT_fclose(fh);
      snprintf(FATShellCustomLangPath,PathnameMaxLength,"%s/lang%d",FATShellCustomPath,ul);
      return;
    }
  }
  
  {
    char fn[PathnameMaxLength];
    u32 ul=1; // default for ENG
    snprintf(fn,PathnameMaxLength,"%s/lang%d/_lang.ini",FATShellCustomPath,ul);
    FAT_FILE *fh=FAT_fopen(fn,"r");
    if(fh!=NULL){
      FAT_fclose(fh);
      snprintf(FATShellCustomLangPath,PathnameMaxLength,"%s/lang%d",FATShellCustomPath,ul);
      return;
    }
  }
  
  FATShellCustomLangPath[0]=0;
}

void Shell_ReadCustomLangFile(const char *fn,void **pbuf,int *psize)
{
  *psize=0;
  *pbuf=NULL;
  
  FAT_FILE *fh;
  
  {
    char fullfn[PathnameMaxLength];
    snprintf(fullfn,PathnameMaxLength,"%s/%s",FATShellCustomLangPath,fn);
    iprintf("load %s\n",fullfn);
    fh=FAT_fopen(fullfn,"r");
  }
  
  if(fh!=NULL){
    FAT_fseek(fh,0,SEEK_END);
    *psize=FAT_ftell(fh);
    FAT_fseek(fh,0,SEEK_SET);
    *pbuf=(void*)safemalloc(*psize);
    
    FAT_fread(*pbuf,1,*psize,fh);
    FAT_fclose(fh);
    return;
  }
}

static bool Shell_EnumMSP_CheckExt(char *fn)
{
  u32 cnt=0;
  
  u32 DotPos=0;
  cnt=0;
  while(fn[cnt]!=0){ 
    if(fn[cnt]=='.') DotPos=cnt;
    cnt++;
  }
  
  if(DotPos==0) return(false);
  
  char c;
  
  c=fn[DotPos+1];
  if((0x41<=c)&&(c<0x5a)) c+=0x20;
  if(c!='m') return(false);
  
  c=fn[DotPos+2];
  if((0x41<=c)&&(c<0x5a)) c+=0x20;
  if(c!='s') return(false);
  
  c=fn[DotPos+3];
  if((0x41<=c)&&(c<0x5a)) c+=0x20;
  if(c!='p') return(false);
  
  return(true);
}

char **Shell_EnumMSP(void)
{
  if(FAT_CWD(FATShellPluginPath)==false) return(NULL);
  
  char fn[FilenameMaxLength];
  u32 FAT_FileType;
  
  char **FileList=(char**)malloc((128+1)*4);
  int StoreIndex=0;
  
  FAT_FileType=FAT_FindFirstFile(fn);
  
  while(FAT_FileType!=FAT_FT_END){ 
    switch(FAT_FileType){ 
      case FAT_FT_DIR: {
      } break;
      case FAT_FT_FILE: {
        if(Shell_EnumMSP_CheckExt(fn)==true){
          FileList[StoreIndex]=(char*)malloc(strlen(fn)+1);
          strcpy(FileList[StoreIndex],fn);
//          iprintf("%s\n",FileList[StoreIndex]);
          StoreIndex++;
        }
      } break;
    }
    
    FAT_FileType=FAT_FindNextFile(fn);
  }
  
  FileList[StoreIndex]=NULL;
  return(FileList);
}

bool Shell_ReadHeadMSP(char *fn,void *buf,int size)
{
  FAT_FILE *fh;
  
  {
    char fullfn[256];
    snprintf(fullfn,256,"%s/%s",FATShellPluginPath,fn);
    fh=FAT_fopen(fullfn,"r");
  }
  
  if(fh!=NULL){
    FAT_fread(buf,1,size,fh);
    FAT_fclose(fh);
    return(true);
  }
  
  return(false);
}

void Shell_ReadMSP(const char *fn,void **pbuf,int *psize)
{
  *pbuf=NULL;
  *psize=0;
  
  FAT_FILE *fh;
  
  {
    char fullfn[256];
    snprintf(fullfn,256,"%s/%s",FATShellPluginPath,fn);
    iprintf("ReadMSP:%s\n",fullfn);
    fh=FAT_fopen(fullfn,"r");
  }
  
  if(fh!=NULL){
    FAT_fseek(fh,0,SEEK_END);
    *psize=FAT_ftell(fh);
    FAT_fseek(fh,0,SEEK_SET);
    *pbuf=(void*)safemalloc(*psize+1);
    ((char*)*pbuf)[*psize]=0;
    
    FAT_fread(*pbuf,1,*psize,fh);
    FAT_fclose(fh);
    return;
  }
}

int Shell_OpenMSP(const char *fn)
{
  char fullfn[256];
  snprintf(fullfn,256,"%s/%s",FATShellPluginPath,fn);
  iprintf("OpenMSP:%s\n",fullfn);
  FAT_FILE *fh=FAT_fopen(fullfn,"r");
  if(fh!=NULL){
    int ifh=FileSys_fopen_DirectMapping(fh);
    return(ifh);
  }
  
  return(0);
}

void Shell_ReadMSPINI(const char *fn,char **pbuf,int *psize)
{
  *pbuf=NULL;
  *psize=0;
  
  char inifn[PathnameMaxLength];
  int fnlen=strlen(fn);
  
  strncpy(inifn,fn,PathnameMaxLength);
  
  inifn[fnlen-3]='I';
  inifn[fnlen-2]='N';
  inifn[fnlen-1]='I';
  
  Shell_ReadMSP(inifn,(void**)pbuf,psize);
  
  return;
}

int Shell_OpenMSPBIN(const char *fn)
{
  char binfn[PathnameMaxLength];
  int fnlen=strlen(fn);
  
  strncpy(binfn,fn,PathnameMaxLength);
  
  binfn[fnlen-3]='B';
  binfn[fnlen-2]='I';
  binfn[fnlen-1]='N';
  
  return(Shell_OpenMSP(binfn));
}

// ----------- MoonShellExecuteBinary

static bool Shell_EnumMSE_CheckExt(char *fn)
{
  u32 cnt=0;
  
  u32 DotPos=0;
  cnt=0;
  while(fn[cnt]!=0){ 
    if(fn[cnt]=='.') DotPos=cnt;
    cnt++;
  }
  
  if(DotPos==0) return(false);
  
  char c;
  
  c=fn[DotPos+1];
  if((0x41<=c)&&(c<0x5a)) c+=0x20;
  if(c!='m') return(false);
  
  c=fn[DotPos+2];
  if((0x41<=c)&&(c<0x5a)) c+=0x20;
  if(c!='s') return(false);
  
  c=fn[DotPos+3];
  if((0x41<=c)&&(c<0x5a)) c+=0x20;
  if(c!='e') return(false);
  
  return(true);
}

char **Shell_EnumMSE(void)
{
  if(FAT_CWD(FATShellPluginPath)==false) return(NULL);
  
  char fn[PathnameMaxLength];
  u32 FAT_FileType;
  
  char **FileList=(char**)malloc((128+1)*4);
  int StoreIndex=0;
  
  FAT_FileType=FAT_FindFirstFile(fn);
  
  while(FAT_FileType!=FAT_FT_END){ 
    switch(FAT_FileType){ 
      case FAT_FT_DIR: {
      } break;
      case FAT_FT_FILE: {
        if(Shell_EnumMSE_CheckExt(fn)==true){
          FileList[StoreIndex]=(char*)malloc(strlen(fn)+1);
          strcpy(FileList[StoreIndex],fn);
//          iprintf("%s\n",FileList[StoreIndex]);
          StoreIndex++;
        }
      } break;
    }
    
    FAT_FileType=FAT_FindNextFile(fn);
  }
  
  FileList[StoreIndex]=NULL;
  return(FileList);
}

u32 Shell_GetMSESize(const char *fn)
{
  FAT_FILE *fh;
  
  {
    char fullfn[PathnameMaxLength];
    snprintf(fullfn,PathnameMaxLength,"%s/%s",FATShellPluginPath,fn);
    fh=FAT_fopen(fullfn,"r");
  }
  
  if(fh!=NULL){
    FAT_fseek(fh,0,SEEK_END);
    u32 size=FAT_ftell(fh);
    FAT_fseek(fh,0,SEEK_SET);
    FAT_fclose(fh);
    return(size);
  }
  
  return(0);
}

bool Shell_GetMSEBody(const char *fn,u32 *pbuf,u32 size)
{
  FAT_FILE *fh;
  
  {
    char fullfn[PathnameMaxLength];
    snprintf(fullfn,PathnameMaxLength,"%s/%s",FATShellPluginPath,fn);
    fh=FAT_fopen(fullfn,"r");
  }
  
  if(fh!=NULL){
    FAT_fread(pbuf,1,size,fh);
    FAT_fclose(fh);
    return(true);
  }
  
  return(false);
}

