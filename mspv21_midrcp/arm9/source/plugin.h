
#ifndef plugin_h
#define plugin_h

#include <stdio.h>

#include <NDS.h>

#include "_consoleWriteLog.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  void (*_consolePrint)(const char *str);
  void (*_consolePrintf)(const char* format, ...);
  void (*_consolePrintSet)(int x, int y);
  void (*ShowLogHalt)(void);
  void (*MWin_ProgressShow)(char *TitleStr,s32 _Max);
  void (*MWin_ProgressSetPos)(s32 _Position);
  void (*MWin_ProgressHide)(void);
  char *(*GetINIData)(void);
  int (*GetINISize)(void);
  void *(*GetBINData)(void);
  int (*GetBINSize)(void);
  
  void (*DC_FlushRangeOverrun)(void *v,u32 size);
  void (*MemCopy8CPU)(void *src,void *dst,u32 len);
  void (*MemCopy16CPU)(void *src,void *dst,u32 len);
  void (*MemCopy32CPU)(void *src,void *dst,u32 len);
  void (*MemSet16CPU)(vu16 v,void *dst,u32 len);
  void (*MemSet32CPU)(u32 v,void *dst,u32 len);
  void (*MemCopy16DMA3)(void *src,void *dst,u32 len);
  void (*MemCopy32DMA3)(void *src,void *dst,u32 len);
  void (*MemSet16DMA3)(vu16 v,void *dst,u32 len);
  void (*MemSet32DMA3)(u32 v,void *dst,u32 len);
  void (*MemSet8DMA3)(u8 v,void *dst,u32 len);
  void (*MemCopy16DMA2)(void *src,void *dst,u32 len);
  void (*MemSet16DMA2)(u16 v,void *dst,u32 len);
  void (*MemCopy32swi256bit)(void *src,void *dst,u32 len);
  void *(*safemalloc)(int size);
  void (*safefree)(void *ptr);
  
  void *(*calloc)(size_t nmemb, size_t size);
  void *(*malloc)(size_t size);
  void (*free)(void *ptr);
  void *(*realloc)(void *ptr, size_t size);
  
  int (*rand)(void);
  
  u32 (*fread)(void *buf, u32 size, u32 n, int fp);
  int (*fseek)(int fp, u32 offset, int origin);
  long (*ftell)(int fp);
  
  int (*sprintf)(char *str, const char *format, ...);
  int (*snprintf)(char *str, size_t size, const char *format, ...);
  
  void *(*memchr)(const void *buf, int ch, size_t n);
  int (*memcmp)(const void *buf1, const void *buf2,size_t n);
  void *(*memcpy)(void *buf1, const void *buf2, size_t n);
  void *(*memmove)(void *buf1, const void *buf2, size_t n);
  void *(*memset)(void *buf, int ch, size_t n);
  
  int (*abs)(int j);
  long int (*labs)(long int j);
  long long int (*llabs)(long long int j);
  double (*fabs)(double x);
  float (*fabsf)(float x);
  
  double (*atof)(const char *nptr);
  int (*atoi)(const char *nptr);
  long (*atol)(const char *nptr);
  long long (*atoll)(const char *nptr);
  
  char *(*strcat)(char *dest, const char *src);
  char *(*strchr)(const char *s, int c);
  int (*strcmp)(const char *s1, const char *s2);
  int (*strcoll)(const char *s1, const char *s2);
  char *(*strcpy)(char *dest, const char *src);
  size_t (*strcspn)(const char *s, const char *reject);
  char *(*strdup)(const char *s);
  size_t (*strlen)(const char *s);
  char *(*strncat)(char *dest, const char *src, size_t n);
  int (*strncmp)(const char *s1, const char *s2, size_t n);
  char *(*strncpy)(char *dest, const char *src, size_t n);
  char *(*strpbrk)(const char *s, const char *accept);
  char *(*strrchr)(const char *s, int c);
  char *(*strsep)(char **stringp, const char *delim);
  size_t (*strspn)(const char *s, const char *accept);
  char *(*strstr)(const char *haystack, const char *needle);
  char *(*strtok)(char *s, const char *delim);
  size_t (*strxfrm)(char *dest, const char *src, size_t n);
  
  int (*GetBINFileHandle)(void);
  int (*msp_fopen)(const char *fn);
  bool (*msp_fclose)(int fh);
} TPlugin_StdLib;

typedef u16 UnicodeChar;

#ifdef PluginMode_Image

typedef struct {
  bool (*Start)(int FileHandle);
  void (*Free)(void);
  
  void (*RefreshScreen)(u32 OffsetX,u32 OffsetY,u16 *pBuf,u32 BufWidth,u32 DrawWidth,u32 DrawHeight,bool HalfMode);
  
  s32 (*GetWidth)(void);
  s32 (*GetHeight)(void);
  
  int (*GetInfoIndexCount)(void);
  bool (*GetInfoStrL)(int idx,char *str,int len);
  bool (*GetInfoStrW)(int idx,UnicodeChar *str,int len);
  bool (*GetInfoStrUTF8)(int idx,char *str,int len);
} TPlugin_ImageLib;

#endif

#ifdef PluginMode_Sound

typedef struct {
  bool (*Start)(int FileHandle);
  void (*Free)(void);
  
  u32 (*Update)(s16 *lbuf,s16 *rbuf);
  
  s32 (*GetPosMax)(void);
  s32 (*GetPosOffset)(void);
  void (*SetPosOffset)(s32 ofs);
  
  u32 (*GetChannelCount)(void);
  u32 (*GetSampleRate)(void);
  u32 (*GetSamplePerFrame)(void);
  int (*GetInfoIndexCount)(void);
  
  bool (*GetInfoStrL)(int idx,char *str,int len);
  bool (*GetInfoStrW)(int idx,UnicodeChar *str,int len);
  bool (*GetInfoStrUTF8)(int idx,char *str,int len);
} TPlugin_SoundLib;

#endif

extern const TPlugin_StdLib *pStdLib;
extern u32 TextDataAddress;
extern int TextDataVector;

extern bool LoadLibrary(const TPlugin_StdLib *_pStdLib);
extern void FreeLibrary(void);
extern int QueryInterfaceLibrary(void);

enum EPluginType {EPT_None=0,EPT_Image=1,EPT_Sound=2};

#ifdef __cplusplus
}
#endif

#endif

