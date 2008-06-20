#include "_console.h"

#include "gba_nds_fat.h"
#include "filesys.h"
#include "shell.h"

int Shell_SkinOpenFile(const char *fn)
{
  FAT_FILE *fh;
  
  {
    char fullfn[PathnameMaxLength];
    //snprintf(fullfn,PathnameMaxLength,"%s/%s",FATShellSkinPath,fn);
    fh=FAT_fopen(fullfn,"r");
  }
  
  /*
  if(fh!=NULL){
    int ifh=FileSys_fopen_DirectMapping(fh);
    return(ifh);
  }
  */
  
  return(0);
}

int Shell_OpenFile(const char *fn)
{
  FAT_FILE *fh;
  
  {
    char fullfn[PathnameMaxLength];
    //snprintf(fullfn,PathnameMaxLength,"%s/%s",FATShellPath,fn);
    fh=FAT_fopen(fullfn,"r");
  }
  
  /*
  if(fh!=NULL){
    int ifh=FileSys_fopen_DirectMapping(fh);
    return(ifh);
  }
  */
  
  return(0);
}
