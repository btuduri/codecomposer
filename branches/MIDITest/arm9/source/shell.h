
#ifndef shell_h
#define shell_h

#include "filesys.h"

extern char FATShellPath[PathnameMaxLength];
extern char FATShellSkinPath[PathnameMaxLength];
extern char FATShellPluginPath[PathnameMaxLength];
extern char FATShellCustomPath[PathnameMaxLength];
extern char FATShellCustomLangPath[PathnameMaxLength];

extern u32 Shell_VolumeType;

extern void Shell_AutoDetect(void);

extern bool Shell_FindShellPath(void);

extern void Shell_ReadFile(const char *fn,void **pbuf,int *psize);
extern int Shell_OpenFile(const char *fn);

extern int Shell_SkinOpenFile(const char *fn);
extern void Shell_ReadSkinFile(const char *fn,void **pbuf,int *psize);

extern void Shell_ReadCustomFile(const char *fn,void **pbuf,int *psize);

extern void Shell_SetCustomLangID(const u32 UserLanguage);
extern void Shell_ReadCustomLangFile(const char *fn,void **pbuf,int *psize);

extern char **Shell_EnumMSP(void);
extern bool Shell_ReadHeadMSP(char *fn,void *buf,int size);
extern void Shell_ReadMSP(const char *fn,void **pbuf,int *psize);
extern void Shell_ReadMSPINI(const char *fn,char **pbuf,int *psize);
extern int Shell_OpenMSPBIN(const char *fn);

extern char **Shell_EnumMSE(void);
extern u32 Shell_GetMSESize(const char *fn);
extern bool Shell_GetMSEBody(const char *fn,u32 *pbuf,u32 size);

#endif

