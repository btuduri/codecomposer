#ifndef _PLUGIN_SUPPLE_H
#define _PLUGIN_SUPPLE_H

typedef struct {
  u32 ID;
  u8 VersionHigh;
  u8 VersionLow;
  u8 PluginType;
  u8 Dummy0;
  u32 DataStart;
  u32 DataEnd;
  u32 gotStart;
  u32 gotEnd;
  u32 bssStart;
  u32 bssEnd;
  u32 LoadLibrary;
  u32 FreeLibrary;
  u32 QueryInterfaceLibrary;
  u32 Dummy1;
  u32 ext[4];
  char info[64];
} TPluginHeader;

typedef struct {
  TPluginHeader PluginHeader;
  void *pData;
  int DataSize;
  void *pbss;
  int bssSize;
  /*
  bool (*LoadLibrary)(const TPlugin_StdLib *_pStdLib,u32 _TextDataAddress);
  void (*FreeLibrary)(void);
  int (*QueryInterfaceLibrary)(void);
  const TPlugin_ImageLib *pIL;
  const TPlugin_SoundLib *pSL;
  const TPlugin_ClockLib *pCL;
  const TPlugin_SoundEffectLib *pSE;
  */
  char *INIData;
  int INISize;
  int BINFileHandle;
  void *BINData;
  int BINSize;
} TPluginBody;

int msp_fopen(const char *fn);
bool msp_fclose(int fh);
char *GetINIData(void);
int GetINISize(void);
void *GetBINData(void);
int GetBINSize(void);
int GetBINFileHandle(void);

/*
static int msp_fopen(const char *fn);
static bool msp_fclose(int fh);
static char *GetINIData(void);
static int GetINISize(void);
static void *GetBINData(void);
static int GetBINSize(void);
static int GetBINFileHandle(void);
*/

#endif