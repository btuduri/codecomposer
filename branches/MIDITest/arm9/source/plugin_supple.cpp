#include "plugin_supple.h"

#include "filesys.h"
#include "memtool.h"
#include "shell.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

static TPluginBody *pCurrentPluginBody = 0;

// Most of related files are located in Dll.cpp (Original Moonshell)
// To do : make the following functions work;
// static int msp_fopen(const char *fn)
int msp_fopen(const char *fn)
{
  iprintf("not support msp_fopen(%s)\n",fn);
  
  return(0);
}

// static bool msp_fclose(int fh)
bool msp_fclose(int fh)
{
  iprintf("not support msp_fclose(%d)\n",fh);
  
  return(false);
}

// static char *GetINIData(void)
char *GetINIData(void)
{
  TPluginBody *pPB=pCurrentPluginBody;
  
  return(pPB->INIData);
}

// static int GetINISize(void)
int GetINISize(void)
{
  TPluginBody *pPB=pCurrentPluginBody;
  
  return(pPB->INISize);
}

// static int GetINISize(void)
void *GetBINData(void)
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

// static int GetBINSize(void)
int GetBINSize(void)
{
  TPluginBody *pPB=pCurrentPluginBody;
  return(pPB->BINSize);
}

// static int GetBINFileHandle(void)
int GetBINFileHandle(void)
{
  char fn[PluginFilenameMax] = "midrcp";

  TPluginBody *pPB=pCurrentPluginBody;
  pPB->BINFileHandle=Shell_OpenMSPBIN(fn);
  
  iprintf("handle is %d\n", pPB->BINFileHandle);

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
  
  return(pPB->BINFileHandle);
}