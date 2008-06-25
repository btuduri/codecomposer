#ifndef _GBA_NDS_FAT_H
#define _GBA_NDS_FAT_H

#include <nds/jtypes.h>

typedef unsigned short int Tunicode;

#define MAX_FILENAME_LENGTH 256	// Maximum LFN length. Don't change this one

// Maximum number of files open at once
// Increase this to open more files, decrease to save memory
#define MAX_FILES_OPEN	8

// File Constants
#ifndef EOF
#define EOF -1
#define SEEK_SET	0
#define SEEK_CUR	1
#define SEEK_END	2
#endif

// Filesystem type
	typedef enum {FS_UNKNOWN, FS_FAT12, FS_FAT16, FS_FAT32} FS_TYPE;

typedef struct
{
	u32 firstCluster;
	u32 length;
	u32 curPos;
	u32 curClus;			// Current cluster to read from
	int curSect;			// Current sector within cluster
	int curByte;			// Current byte within sector
	char readBuffer[512];	// Buffer used for unaligned reads
	u32 appClus;			// Cluster to append to
	int appSect;			// Sector within cluster for appending
	int appByte;			// Byte within sector for appending
	bool read;	// Can read from file
	bool write;	// Can write to file
	bool append;// Can append to file
	bool inUse;	// This file is open
	u32 dirEntSector;	// The sector where the directory entry is stored
	int dirEntOffset;	// The offset within the directory sector
}	FAT_FILE;



//-----------------------------------------------------------------
// File functions

/*-----------------------------------------------------------------
FAT_fopen(filename, mode)
Opens a file
const char* path: IN null terminated string of filename and path 
	separated by forward slashes, / is root
const char* mode: IN mode to open file in
	Supported modes: "r", "r+", "w", "w+", "a", "a+", don't use
	"b" or "t" in any mode, as all files are openned in binary mode
FAT_FILE* return: OUT handle to open file, returns -1 if the file 
	couldn't be openned
-----------------------------------------------------------------*/
FAT_FILE* FAT_fopen(const char* path, const char* mode);

#endif
