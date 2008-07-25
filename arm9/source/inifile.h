#ifndef inifile_h
#define inifile_h

#include <nds/jtypes.h>

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
} TMIDIINI;

// Following code has been imported from inifile.h, which is located in Moonshell
enum EGMEPluginSimpleLPF {EGMESimpleLPF_None=0,EGMESimpleLPF_Lite=1,EGMESimpleLPF_Heavy=2};
enum EClosedSholderButton {ECSB_Disabled=0,ECSB_Flexible=1,ECSB_AlwaysDisabled=2,ECSB_Enabled=3};
enum EiniSystemMusicNext {EISMN_Stop=0,EISMN_Repeat=1,EISMN_NormalLoop=2,EISMN_NormalPOff=3,EISMN_ShuffleLoop=4,EISMN_ShufflePOff=5,EISMN_PowerOff=6,EISMN_Count=7};
enum EiniSystemWhenPanelClose {EISWPC_BacklightOff=0,EISWPC_DSPowerOff=1,EISWPC_PlayShutdownSound=2};
enum EScreenFlip {ESF_Normal=0,ESF_Flip=1,ESF_VFlip=2,WFS_HFlip=3};
enum EiniStartButtonExec {EISBE_ShowDebugLog=0,EISBE_SoftReset=1};

typedef struct {
  bool hiddenAboutWindow;
  bool hiddenHelpWindow;
  bool hiddenDateTimeWindow;
  u32 StartupSound;
} TiniBoot;

typedef struct {
  u32 WhenStandby;
  u32 WhenPicture;
  u32 WhenText;
  u32 WhenSound;
} TiniBacklightTimeout;

typedef struct {
  u32 PageIndex,PageOffset,ExitProcIndex;
} TiniCustom;

typedef struct {
  bool Attribute_Archive;
  bool Attribute_Hidden;
  bool Attribute_System;
  bool Attribute_Readonly;
  bool Path_Shell;
  bool Path_Moonshl;
  bool File_Thumbnail;
} TiniHiddenItem;

typedef struct {
    u32 ROM1stAccessCycleControl;
    u32 ROM2stAccessCycleControl;
} TiniForSuperCard;

typedef struct {
  bool FileSelectSubScreen;
  enum EClosedSholderButton ClosedSholderButton;
  enum EiniSystemMusicNext MusicNext;
  int SoundVolume;
  bool FullScreenOverlaySubScreen;
  u32 FileMaxCount;
  u32 NDSLiteDefaultBrightness;
  enum EiniSystemWhenPanelClose WhenPanelClose;
  enum EScreenFlip TopScreenFlip;
  bool ResumeUsingWhileMusicPlaying;
  bool LockLRButton;
  enum EiniStartButtonExec StartButtonExec;
  bool PrivateFunction;
} TiniSystem;

typedef struct {
  u32 DelayCount;
  u32 RateCount;
} TiniKeyRepeat;

typedef struct {
  u32 ReverbLevel;
  enum EGMEPluginSimpleLPF SimpleLPF;
  u32 DefaultLengthSec;
  u32 HES_MaxTrackNumber;
} TiniGMEPlugin;

typedef struct {
  bool SC_EnabledDRAM;
  u32 EZ4_PSRAMSizeMByte;
} TiniAdapterConfig;

typedef struct {
  TiniCustom Custom;
  TiniSystem System;
  TiniForSuperCard ForSuperCard;
  TiniHiddenItem HiddenItem;
  TiniKeyRepeat KeyRepeat;
  TiniBacklightTimeout BacklightTimeout;
  TiniBoot Boot;
  /*
  TiniThumbnail Thumbnail;
  TiniTextPlugin TextPlugin;
  TiniNDSROMPlugin NDSROMPlugin;
  TiniDPGPlugin DPGPlugin;
  TiniImagePlugin ImagePlugin;
  TiniClockPlugin ClockPlugin;
  TiniOverrideWindowRect OverrideWindowRect;
  TiniGMEPlugin GMEPlugin;
  */
  TiniAdapterConfig AdapterConfig;
} TGlobalINI;

extern TGlobalINI GlobalINI;
extern TMIDIINI MIDIINI;

extern void InitINI(void);
extern void LoadINI(char *data,int size);

#endif