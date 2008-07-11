#ifndef _PLUGIN_SUPPLE_H
#define _PLUGIN_SUPPLE_H

#include <nds/jtypes.h>

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

#define PluginFilenameMax (16)

#endif