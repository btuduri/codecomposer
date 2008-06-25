#ifndef _DISC_IO_H
#define _DISC_IO_H

#include <nds/jtypes.h>

//#define DISC_CACHE				// uncomment this line to enable disc caching
#define DISC_CACHE_COUNT	256	// maximum number of sectors to cache (512 bytes per sector)

#define BYTE_PER_READ 512

/*
#define disc_ReadSector(sector,buffer)	disc_ReadSectors(sector,1,buffer)
#define disc_WriteSector(sector,buffer)	disc_WriteSectors(sector,1,buffer)
*/

bool disc_ReadSectors(u32 sector, u8 numSecs, void* buffer);
bool disc_WriteSectors(u32 sector, u8 numSecs, void* buffer); 

bool disc_ReadSector(u32 sector, void* buffer);
bool disc_WriteSector(u32 sector, void* buffer); 

typedef bool (* FN_MEDIUM_STARTUP)(void) ;
typedef bool (* FN_MEDIUM_ISINSERTED)(void) ;
typedef bool (* FN_MEDIUM_READSECTORS)(u32 sector, u8 numSecs, void* buffer) ;
typedef bool (* FN_MEDIUM_WRITESECTORS)(u32 sector, u8 numSecs, void* buffer) ;
typedef bool (* FN_MEDIUM_CLEARSTATUS)(void) ;
typedef bool (* FN_MEDIUM_SHUTDOWN)(void) ;

typedef struct {
	unsigned long			ul_ioType ;
	unsigned long			ul_Features ;
	FN_MEDIUM_STARTUP		fn_StartUp ;
	FN_MEDIUM_ISINSERTED	fn_IsInserted ;
	FN_MEDIUM_READSECTORS	fn_ReadSectors ;
	FN_MEDIUM_WRITESECTORS	fn_WriteSectors ;
	FN_MEDIUM_CLEARSTATUS	fn_ClearStatus ;
	FN_MEDIUM_SHUTDOWN		fn_Shutdown ;
} IO_INTERFACE, *LPIO_INTERFACE ;

#endif
