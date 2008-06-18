#include <nds.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "filesys.h"
#include "inifile.h"

#include "_console.h"

#include "_consoleWriteLog.h"

#include "gba_nds_fat/gba_nds_fat.h"

typedef struct {
  bool Enabled;
  FAT_FILE *file;
} TMemFile;

#define MemFileCount (8)

static TMemFile MemFile[MemFileCount];

long int FileSys_ftell (int hFile)
{ //cwl();
  TMemFile *pMemFile=&MemFile[hFile];
  if(pMemFile->Enabled==false) return(0);
  
  return(FAT_ftell(pMemFile->file));
}

int FileSys_fseek(int hFile, u32 offset, int origin)
{ //cwl();
  TMemFile *pMemFile=&MemFile[hFile];
  if(pMemFile->Enabled==false) return(0);
  
  return(FAT_fseek(pMemFile->file,offset,origin));
}

u32 FileSys_fread (void* buffer, u32 size, u32 count, int hFile)
{ //cwl();
  TMemFile *pMemFile=&MemFile[hFile];
  if(pMemFile->Enabled==false) return(0);
  
  return(FAT_fread(buffer,size,count,pMemFile->file));
}
