
// However, the following part was in the main.cpp(Original moonshell)
// To do : make the following functions work;
#define ShowLogHalt() { \
  _ShowLogHalt(); \
  while(1); \
}

__declspec(noreturn) void _ShowLogHalt(void)
{ cwl();
  REG_IME=0;
  
  videoSetModeSub_SetShowLog(true);
  IPC3->LCDPowerControl=LCDPC_ON_BOTH;
  _consolePrint("\n");
  while(1){ cwl();
    swiWaitForVBlank();
  }
}

// Most of related files are located in Dll.cpp (Original Moonshell)
// To do : make the following functions work;
static int msp_fopen(const char *fn)
{
  _consolePrintf("not support msp_fopen(%s)\n",fn);
  ShowLogHalt();
  return(0);
}

static bool msp_fclose(int fh)
{
  _consolePrintf("not support msp_fclose(%d)\n",fh);
  ShowLogHalt();
  return(false);
}

static char *GetINIData(void)
{
  TPluginBody *pPB=pCurrentPluginBody;
  
  return(pPB->INIData);
}

static int GetINISize(void)
{
  TPluginBody *pPB=pCurrentPluginBody;
  
  return(pPB->INISize);
}

static void *GetBINData(void)
{
  TPluginBody *pPB=pCurrentPluginBody;
  
  if(pPB->BINFileHandle==0) return(0);
  if(pPB->BINSize==0) return(0);
  
  if(pPB->BINData==NULL){
    pPB->BINData=safemalloc(pPB->BINSize);
    if(pPB->BINData!=NULL){
      FileSys_fread(pPB->BINData,1,pPB->BINSize,pPB->BINFileHandle);
    }
  }
  
  return(pPB->BINData);
}

static int GetBINSize(void)
{
  TPluginBody *pPB=pCurrentPluginBody;
  
  return(pPB->BINSize);
}

static int GetBINFileHandle(void)
{
  TPluginBody *pPB=pCurrentPluginBody;
  
  if(pPB->BINFileHandle==0) return(0);
  if(pPB->BINSize==0) return(0);
  
  return(pPB->BINFileHandle);
}