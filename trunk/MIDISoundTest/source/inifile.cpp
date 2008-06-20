#include "inifile.h"

TGlobalINI GlobalINI;

void InitINI(void)
{
  {
    TiniMIDPlugin *MIDPlugin=&GlobalINI.MIDPlugin;
    MIDPlugin->SampleRate=32768;
    MIDPlugin->FramePerSecond=90;
    MIDPlugin->ShowEventMessage=false;
    MIDPlugin->MaxVoiceCount=32;
    MIDPlugin->GenVolume=100;
    MIDPlugin->ReverbFactor_ToneMap=64;
    MIDPlugin->ReverbFactor_DrumMap=32;
    MIDPlugin->DelayStackSize=12;
    MIDPlugin->ShowInfomationMessages=true;
  }
}

static char section[128];
static u32 readline;

static void readsection(char *str)
{
  str++;
  
  u32 ofs;
  
  ofs=0;
  while(*str!=']'){
    if((128<=ofs)||(*str==0)){
      _consolePrintf("line%d error.\nThe section name doesn't end correctly.\n",readline);

//	  ShowLogHalt();	// This function cannot be define nor make by ourselves.
    }
    section[ofs]=*str;
    str++;
    ofs++;
  }
  section[ofs]=0;
}

static void readkey(char *str)
{
  if(section[0]==0){
    _consolePrintf("line%d error.\nThere is a key ahead of the section name.\n",readline);
//    ShowLogHalt();	// This function cannot be define nor make by ourselves.
    return;
  }
  
  char key[128],value[128];
  
  u32 ofs;
  
  ofs=0;
  while(*str!='='){
    if((128<=ofs)||(*str==0)){
      _consolePrintf("line%d error.\nThe key name doesn't end correctly.\n",readline);
//      ShowLogHalt();	// This function cannot be define nor make by ourselves.
    }
    key[ofs]=*str;
    str++;
    ofs++;
  }
  key[ofs]=0;
  
  str++;
  
  ofs=0;
  while(*str!=0){
    if(128<=ofs){
      _consolePrintf("line%d error.\nThe value doesn't end correctly.\n",readline);
//     ShowLogHalt();	// This function cannot be define nor make by ourselves.
    }
    value[ofs]=*str;
    str++;
    ofs++;
  }
  value[ofs]=0;
  
  s32 ivalue=atoi(value);
  bool bvalue;
  
  if(ivalue==0){
    bvalue=false;
    }else{
    bvalue=true;
  }
  
  bool match=false;
  
  if((match==false)&&(strcmp(section,"MIDPlugin")==0)){
    TiniMIDPlugin *MIDPlugin=&GlobalINI.MIDPlugin;
    
    if((match==false)&&(strcmp(key,"SampleRate")==0)){
      match=true;
      MIDPlugin->SampleRate=ivalue;
    }
    if((match==false)&&(strcmp(key,"FramePerSecond")==0)){
      match=true;
      MIDPlugin->FramePerSecond=ivalue;
    }
    if((match==false)&&(strcmp(key,"ShowEventMessage")==0)){
      match=true;
      MIDPlugin->ShowEventMessage=bvalue;
    }
    if((match==false)&&(strcmp(key,"MaxVoiceCount")==0)){
      match=true;
      MIDPlugin->MaxVoiceCount=ivalue;
    }
    if((match==false)&&(strcmp(key,"GenVolume")==0)){
      match=true;
      MIDPlugin->GenVolume=ivalue;
    }
    if((match==false)&&(strcmp(key,"ReverbFactor_ToneMap")==0)){
      match=true;
      MIDPlugin->ReverbFactor_ToneMap=ivalue;
    }
    if((match==false)&&(strcmp(key,"ReverbFactor_DrumMap")==0)){
      match=true;
      MIDPlugin->ReverbFactor_DrumMap=ivalue;
    }
    if((match==false)&&(strcmp(key,"DelayStackSize")==0)){
      match=true;
      MIDPlugin->DelayStackSize=ivalue;
    }
    if((match==false)&&(strcmp(key,"ShowInfomationMessages")==0)){
      match=true;
      MIDPlugin->ShowInfomationMessages=bvalue;
    }
    
  }
  
  if(match==false){
    _consolePrintf("line%d error.\ncurrent section [%s] unknown key=%s\n",readline,section,key);
//    ShowLogHalt();
  }
  
//  _consolePrintf("key=%s value=%s\n",key,value);
}

static void internal_LoadGlobalINI(char *pini,u32 inisize)
{
  section[0]=0;
  readline=0;
  
  u32 iniofs=0;
  
  while(iniofs<inisize){
    
    readline++;
    
    u32 linelen=0;
    
    // Calc Line Length
    {
      char *s=&pini[iniofs];
      
      while(0x20<=*s){
        linelen++;
        s++;
        if(inisize<=(iniofs+linelen)) break;
      }
      *s=0;
    }
    
    if(linelen!=0){
      char c=pini[iniofs];
      if((c==';')||(c=='/')||(c=='!')){
        // comment line
        }else{
        if(c=='['){
          readsection(&pini[iniofs]);
          }else{
          readkey(&pini[iniofs]);
        }
      }
    }
    
    iniofs+=linelen;
    
    // skip NULL,CR,LF
    {
      char *s=&pini[iniofs];
      
      while(*s<0x20){
        iniofs++;
        s++;
        if(inisize<=iniofs) break;
      }
    }
    
  }
}

void LoadINI(char *data,int size)
{
  if((data==NULL)||(size==0)) return;
  
  internal_LoadGlobalINI(data,size);
}

