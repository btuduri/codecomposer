
#ifndef filesys_h
#define filesys_h

#include "gba_nds_fat.h"
#include "mediatype.h"

#include "unicode.h"

#define MPCFIOTitle "External file system I/O"

#define MPCFIOTitle0 "gba_nds_fat by Chishm"
#define MPCFIOTitle1 "GBAMP CF I/O test r9 by SaTa."

#define FT_Path (0)
#define FT_File (1)

#define PathnameMaxLength (256)
#define AliasMaxLength (13+1)
#define FilenameMaxLength (128)
#define ExtMaxLength (8)

#define SystemFileID_Thumbnail1 (0xffffff01)
#define SystemFilename_Thumbnail1 ("_THUMBNL.MSL")

extern u32 FileSys_VolumeType;
extern char FileSys_PathName[PathnameMaxLength];

extern char *FileSys_InitInterface_ErrorStr;
extern bool FileSys_InitInterface(void);
extern void FileSys_FreeInterface(void);

#ifdef __cplusplus
extern "C" {
#endif

void FileSys_Init(int _FileMaxCount);
void FileSys_Free(void);

void FileSys_RefreshPath(void);
void FileSys_ChangePath(const char *TargetPathName);
bool FileSys_isPathExists(const char *TargetPathName);

char* FileSys_GetPathName(void);
s32 FileSys_GetLastPathItemIndex(void);

s32 FileSys_GetFileCount(void);
s32 FileSys_GetPureFileCount(void);

char* FileSys_GetAlias(s32 FileIndex);
UnicodeChar* FileSys_GetFilename(s32 FileIndex);
u32 FileSys_GetFileType(s32 FileIndex);
int FileSys_GetFileTrackNum(s32 FileIndex);
void FileSys_GetFileExt(s32 FileIndex,char *ext);
u32 FileSys_GetFileDataSize(u32 FileIndex);
bool FileSys_GetFileData(u32 FileIndex,u8 *dstbuf);

int FileSys_fopen_DirectMapping(FAT_FILE *file);
int FileSys_fopen(u32 FileIndex);
bool FileSys_fclose (int hFile);
long int FileSys_ftell (int hFile);
int FileSys_fseek(int hFile, u32 offset, int origin);
u32 FileSys_fread (void* buffer, u32 size, u32 count, int hFile);
u32 FileSys_fread_fast (void* buffer, u32 size, u32 count, int hFile);
FAT_FILE *FileSys_Get_GBANDSFAT_FileHandle(int hFile);

void Shuffle_Clear(void);
int Shuffle_GetNextIndex(int LastIndex);
int Shuffle_GetPrevIndex(int LastIndex);
int Normal_GetNextIndex(int LastIndex);
int Normal_GetPrevIndex(int LastIndex);

#ifdef __cplusplus
}	   // extern "C"
#endif

#endif

