
#ifndef directdisk_h
#define directdisk_h

#include <NDS.h>

#include "gba_nds_fat.h"

#define DD_SectorSize (512)

enum EDDFile {EDDFile_resume=0,EDDFile_setting,EDDFile_bookmrk0,EDDFile_bookmrk1,EDDFile_bookmrk2,EDDFile_bookmrk3,EDDFile_MaxCount};

enum EDDSaveType {EDDST_None,EDDST_FAT};

extern bool DD_isEnabled(void);
extern void DD_Init(EDDSaveType st);
extern EDDSaveType DD_GetSaveType(void);
extern bool DD_isExists(EDDFile DDFile);
extern bool DD_InitFile(EDDFile DDFile);
extern void DD_ReadFile(EDDFile DDFile,void *pbuf,u32 bufsize);
extern void DD_WriteFile(EDDFile DDFile,void *pbuf,u32 bufsize);

#endif

