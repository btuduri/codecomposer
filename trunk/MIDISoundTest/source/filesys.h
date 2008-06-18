#ifndef _FILESYS_H
#define _FILESYS_H

#define PathnameMaxLength (256)
#define AliasMaxLength (13+1)
#define FilenameMaxLength (128)
#define ExtMaxLength (8)

long int FileSys_ftell (int hFile);
int FileSys_fseek(int hFile, u32 offset, int origin);
u32 FileSys_fread (void* buffer, u32 size, u32 count, int hFile);

#endif
