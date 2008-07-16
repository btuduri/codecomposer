#include "plugin_supple.h"

#include "filesys.h"
#include "memtool.h"
#include "shell.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

// Most of related files are located in Dll.cpp (Original Moonshell)
// To do : make the following functions work;
// static int msp_fopen(const char *fn)
int msp_fopen(const char *fn)
{
  iprintf("not support msp_fopen(%s)\n",fn);
  
  return(0);
}

bool msp_fclose(int fh)
{
  iprintf("not support msp_fclose(%d)\n",fh);
  
  return(false);
}

char *GetINIData(TPluginBody *pCurrentPluginBody)
{
  TPluginBody *pPB=pCurrentPluginBody;
  
  return(pPB->INIData);
}

int GetINISize(TPluginBody *pCurrentPluginBody)
{
  TPluginBody *pPB=pCurrentPluginBody;
  
  return(pPB->INISize);
}

void *GetBINData(TPluginBody *pCurrentPluginBody)
{
  TPluginBody *pPB=pCurrentPluginBody;
  
  if(pPB->BINFileHandle==0) return(0);
  if(pPB->BINSize==0) return(0);
  
  if(pPB->BINData==0){
    pPB->BINData=safemalloc(pPB->BINSize);
    if(pPB->BINData!=0){
      FileSys_fread(pPB->BINData,1,pPB->BINSize,pPB->BINFileHandle);
    }
  }
  
  return(pPB->BINData);
}

int GetBINSize(TPluginBody *pCurrentPluginBody)
{
  TPluginBody *pPB=pCurrentPluginBody;
  return(pPB->BINSize);
}


int GetBINFileHandle(TPluginBody *pCurrentPluginBody)
{
  char fn[PluginFilenameMax] = "midrcp.bin";
  TPluginBody *pPB = pCurrentPluginBody;
  pPB->BINFileHandle=Shell_OpenFile(fn);

  iprintf("before bin handle is %d\n", pPB->BINFileHandle);

  if(pPB->BINFileHandle==0)
  {
    pPB->BINData=NULL;
    pPB->BINSize=0;
  }
  else
  {
    pPB->BINData=NULL;
    FileSys_fseek(pPB->BINFileHandle,0,SEEK_END);
    pPB->BINSize=FileSys_ftell(pPB->BINFileHandle);
    FileSys_fseek(pPB->BINFileHandle,0,SEEK_SET);
  }

  iprintf("after bin handle is %d\n", pPB->BINFileHandle);

  return(pPB->BINFileHandle);
}