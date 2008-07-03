#include "shell.h"
#include "gba_nds_fat.h"
#include "filesys.h"


int Shell_SkinOpenFile(const char *fn)
{
  FAT_FILE *fh;
  
  {
    char fullfn[PathnameMaxLength];
//    snprintf(fullfn,PathnameMaxLength,"%s/%s",FATShellSkinPath,fn);
    fh=FAT_fopen(fullfn,"r");
  }
  
//  if(fh!=NULL){
//	_consolePrintf("Test");
    //int ifh=FileSys_fopen_DirectMapping(fh);
//    return 0;//(fh);
//  }
    
  return(0);
}
