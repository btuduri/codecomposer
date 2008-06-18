#include "_console.h"

#include "gba_nds_fat/gba_nds_fat.h"
#include "filesys.h"

#include "shell.h"

#if 0
void Shell_ReadMSP(const char *fn,void **pbuf,int *psize)
{
  *pbuf=NULL;
  *psize=0;
  
  FAT_FILE *fh;
  
  {
    char fullfn[256];
    snprintf(fullfn,256,"%s/%s",FATShellPluginPath,fn);
    _consolePrintf("ReadMSP:%s\n",fullfn);
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
#endif

int Shell_SkinOpenFile(const char *fn)
{
  FAT_FILE *fh;
  
  {
    char fullfn[PathnameMaxLength];
    //snprintf(fullfn,PathnameMaxLength,"%s/%s",FATShellSkinPath,fn);
    fh=FAT_fopen(fullfn,"r");
  }
  
  /*
  if(fh!=NULL){
    int ifh=FileSys_fopen_DirectMapping(fh);
    return(ifh);
  }
  */
  
  return(0);
}

int Shell_OpenFile(const char *fn)
{
  FAT_FILE *fh;
  
  {
    char fullfn[PathnameMaxLength];
    //snprintf(fullfn,PathnameMaxLength,"%s/%s",FATShellPath,fn);
    fh=FAT_fopen(fullfn,"r");
  }
  
  /*
  if(fh!=NULL){
    int ifh=FileSys_fopen_DirectMapping(fh);
    return(ifh);
  }
  */
  
  return(0);
}
