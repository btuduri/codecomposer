#ifndef inifile_h
#define inifile_h

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <NDS.h>

#include "_console.h"

typedef struct {
  u32 SampleRate;
  u32 FramePerSecond;
  bool ShowEventMessage;
  u32 MaxVoiceCount;
  u32 GenVolume;
  u32 ReverbFactor_ToneMap;
  u32 ReverbFactor_DrumMap;
  u32 DelayStackSize;
  bool ShowInfomationMessages;
} TiniMIDPlugin;

typedef struct {
  TiniMIDPlugin MIDPlugin;
} TGlobalINI;

extern TGlobalINI GlobalINI;

extern void InitINI(void);
extern void LoadINI(char *data,int size);

#endif


