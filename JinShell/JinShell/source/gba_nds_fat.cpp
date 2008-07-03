#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "gba_nds_fat.h"

#include "disc_io.h"

//---------------------------------------------------------------
// Appropriate placement of CF functions and data
#ifdef NDS
 #define _VARS_IN_RAM 
#else
 #define _VARS_IN_RAM __attribute__ ((section (".sbss")))
#endif


//-----------------------------------------------------------------
// FAT constants
#define CLUSTER_EOF_16	0xFFFF
#define	CLUSTER_EOF	0x0FFFFFFF
#define CLUSTER_FREE	0x0000
#define CLUSTER_FIRST	0x0002

#define FILE_LAST 0x00
#define FILE_FREE 0xE5

#define ATTRIB_ARCH	0x20
#define ATTRIB_DIR	0x10
#define ATTRIB_LFN	0x0F
#define ATTRIB_VOL	0x08
#define ATTRIB_HID	0x02
#define ATTRIB_SYS	0x04
#define ATTRIB_RO	0x01

#define FAT16_ROOT_DIR_CLUSTER 0x00

//-----------------------------------------------------------------
// long file name constants
#define LFN_END 0x40
#define LFN_DEL 0x80

// Take care of packing for GCC - it doesn't obey pragma pack()
// properly for ARM targets.
#ifdef __GNUC__
 #define __PACKED __attribute__ ((__packed__))
#else
 #define __PACKED 
 #pragma pack(1)
#endif


// Directory entry - must be packed
typedef struct
{
	__PACKED	u8	name[8];
	__PACKED	u8	ext[3];
	__PACKED	u8	attrib;
	__PACKED	u8	reserved;
	__PACKED	u8	cTime_ms;
	__PACKED	u16	cTime;
	__PACKED	u16	cDate;
	__PACKED	u16	aDate;
	__PACKED	u16	startClusterHigh;
	__PACKED	u16	mTime;
	__PACKED	u16	mDate;
	__PACKED	u16	startCluster;
	__PACKED	u32	fileSize;
}	DIR_ENT;

// Long file name directory entry - must be packed
typedef struct
{
	__PACKED	u8 ordinal;	// Position within LFN
	__PACKED	u16 char0;	
	__PACKED	u16 char1;
	__PACKED	u16 char2;
	__PACKED	u16 char3;
	__PACKED	u16 char4;
	__PACKED	u8 flag;	// Should be equal to ATTRIB_LFN
	__PACKED	u8 reserved1;	// Always 0x00
	__PACKED	u8 checkSum;	// Checksum of short file name (alias)
	__PACKED	u16 char5;
	__PACKED	u16 char6;
	__PACKED	u16 char7;
	__PACKED	u16 char8;
	__PACKED	u16 char9;
	__PACKED	u16 char10;
	__PACKED	u16 reserved2;	// Always 0x0000
	__PACKED	u16 char11;
	__PACKED	u16 char12;
}	DIR_ENT_LFN;

static _VARS_IN_RAM Tunicode lfnNameUnicode[MAX_FILENAME_LENGTH];

// Files
_VARS_IN_RAM FAT_FILE openFiles[MAX_FILES_OPEN];

// Long File names
_VARS_IN_RAM char lfnName[MAX_FILENAME_LENGTH];

bool lfnExists;

// Locations on card
int filesysRootDir;
int filesysRootDirClus;
int filesysFAT;
int filesysSecPerFAT;
int filesysNumSec;
int filesysData;
int filesysBytePerSec;
int filesysSecPerClus;
int filesysBytePerClus;

FS_TYPE filesysType = FS_UNKNOWN;

// Info about FAT
u32 fatLastCluster;
u32 fatFirstFree;


// fatBuffer used to reduce wear on the CF card from multiple writes
_VARS_IN_RAM char fatBuffer[BYTE_PER_READ];
u32 fatBufferCurSector;

// Current working directory
u32 curWorkDirCluster;

// Position of the directory entry last retreived with FAT_GetDirEntry
u32 wrkDirCluster;
int wrkDirSector;
int wrkDirOffset;

// Global sector buffer to save on stack space
_VARS_IN_RAM unsigned char globalBuffer[BYTE_PER_READ];

char ucase (char character)
{
	if ((character > 0x60) && (character < 0x7B))
		character = character - 0x20;
	return (character);
}

/*-----------------------------------------------------------------
Disc level FAT routines
-----------------------------------------------------------------*/
#define FAT_ClustToSect(m) \
	(((m-2) * filesysSecPerClus) + filesysData)


/*-----------------------------------------------------------------
FAT_NextCluster
Internal function - gets the cluster linked from input cluster
-----------------------------------------------------------------*/
u32 FAT_NextCluster(u32 cluster)
{
	u32 nextCluster = CLUSTER_FREE;
	u32 sector;
	int offset;
	
	switch (filesysType) 
	{
		case FS_UNKNOWN:
			nextCluster = CLUSTER_FREE;
			break;
			
		case FS_FAT12:
			sector = filesysFAT + (((cluster * 3) / 2) / BYTE_PER_READ);
			offset = ((cluster * 3) / 2) % BYTE_PER_READ;

			// If FAT buffer contains wrong sector
			if (sector != fatBufferCurSector)
			{
				// Load correct sector to buffer
				fatBufferCurSector = sector;
				disc_ReadSector(fatBufferCurSector, fatBuffer);
			}

			nextCluster = ((u8*)fatBuffer)[offset];
			offset++;
			
			if (offset >= BYTE_PER_READ) {
				offset = 0;
				fatBufferCurSector++;
				disc_ReadSector(fatBufferCurSector, fatBuffer);
			}
			
			nextCluster |= (((u8*)fatBuffer)[offset]) << 8;
			
			if (cluster & 0x01) {
				nextCluster = nextCluster >> 4;
			} else 	{
				nextCluster &= 0x0FFF;
			}
			
			if (nextCluster >= 0x0FF7)
			{
				nextCluster = CLUSTER_EOF;
			}

			break;
			
		case FS_FAT16:
#ifdef UseFatDump
#include "gba_nds_fat_chg/nextcluster.h"
#else
			sector = filesysFAT + ((cluster << 1) / BYTE_PER_READ);
			offset = cluster % (BYTE_PER_READ >> 1);
			
			// If FAT buffer contains wrong sector
			if (sector != fatBufferCurSector)
			{
				// Load correct sector to buffer
				fatBufferCurSector = sector;
				disc_ReadSector(fatBufferCurSector, fatBuffer);
			}

			// read the nextCluster value
			nextCluster = ((u16*)fatBuffer)[offset];
#endif
			
			if (nextCluster >= 0xFFF7)
			{
				nextCluster = CLUSTER_EOF;
			}
			break;
			
		case FS_FAT32:
			sector = filesysFAT + ((cluster << 2) / BYTE_PER_READ);
			offset = cluster % (BYTE_PER_READ >> 2);
			
			// If FAT buffer contains wrong sector
			if (sector != fatBufferCurSector)
			{
				// Load correct sector to buffer
				fatBufferCurSector = sector;
				disc_ReadSector(fatBufferCurSector, fatBuffer);
			}

			// read the nextCluster value
			nextCluster = (((u32*)fatBuffer)[offset]) & 0x0FFFFFFF;
			
			if (nextCluster >= 0x0FFFFFF7)
			{
				nextCluster = CLUSTER_EOF;
			}
			break;
			
		default:
			nextCluster = CLUSTER_FREE;
			break;
	}
	
	return nextCluster;
}

/*-----------------------------------------------------------------
FAT_WriteFatEntry
Internal function - writes FAT information about a cluster
-----------------------------------------------------------------*/
bool FAT_WriteFatEntry (u32 cluster, u32 value)
{
	u32 sector;
	int offset;

	if ((cluster < 0x0002) || (cluster > fatLastCluster))
	{
		return false;
	}
	
	switch (filesysType) 
	{
		case FS_UNKNOWN:
			return false;
			break;
			
		case FS_FAT12:
			sector = filesysFAT + (((cluster * 3) / 2) / BYTE_PER_READ);
			offset = ((cluster * 3) / 2) % BYTE_PER_READ;

			// If FAT buffer contains wrong sector
			if (sector != fatBufferCurSector)
			{
				// Load correct sector to buffer
				fatBufferCurSector = sector;
				disc_ReadSector(fatBufferCurSector, fatBuffer);
			}

			if (cluster & 0x01) {

				((u8*)fatBuffer)[offset] = (((u8*)fatBuffer)[offset] & 0x0F) | ((value & 0x0F) << 4);

				offset++;
				if (offset >= BYTE_PER_READ) {
					offset = 0;
					// write the buffer back to disc
					disc_WriteSector(fatBufferCurSector, fatBuffer);
					// read the next sector	
					fatBufferCurSector++;
					disc_ReadSector(fatBufferCurSector, fatBuffer);
				}
				
				((u8*)fatBuffer)[offset] =  (value & 0x0FF0) >> 4;

			} else {
			
				((u8*)fatBuffer)[offset] = value & 0xFF;
		
				offset++;
				if (offset >= BYTE_PER_READ) {
					offset = 0;
					// write the buffer back to disc
					disc_WriteSector(fatBufferCurSector, fatBuffer);
					// read the next sector	
					fatBufferCurSector++;
					disc_ReadSector(fatBufferCurSector, fatBuffer);
				}
				
				((u8*)fatBuffer)[offset] = (((u8*)fatBuffer)[offset] & 0xF0) | ((value >> 8) & 0x0F);
			}

			break;
			
		case FS_FAT16:
			sector = filesysFAT + ((cluster << 1) / BYTE_PER_READ);
			offset = cluster % (BYTE_PER_READ >> 1);

			// If FAT buffer contains wrong sector
			if (sector != fatBufferCurSector)
			{
				// Load correct sector to buffer
				fatBufferCurSector = sector;
				disc_ReadSector(fatBufferCurSector, fatBuffer);
			}

			// write the value to the FAT buffer
			((u16*)fatBuffer)[offset] = (value & 0xFFFF);

			break;
			
		case FS_FAT32:
			sector = filesysFAT + ((cluster << 2) / BYTE_PER_READ);
			offset = cluster % (BYTE_PER_READ >> 2);
			
			// If FAT buffer contains wrong sector
			if (sector != fatBufferCurSector)
			{
				// Load correct sector to buffer
				fatBufferCurSector = sector;
				disc_ReadSector(fatBufferCurSector, fatBuffer);
			}

			// write the value to the FAT buffer
			(((u32*)fatBuffer)[offset]) =  value;

			break;
			
		default:
			return false;
			break;
	}
	
	// write the buffer back to disc
	disc_WriteSector(fatBufferCurSector, fatBuffer);
			
	return true;
}


/*-----------------------------------------------------------------
FAT_FirstFreeCluster
Internal function - gets the first available free cluster
-----------------------------------------------------------------*/
u32 FAT_FirstFreeCluster(void)
{
	// Start at first valid cluster
	if (fatFirstFree < CLUSTER_FIRST)
		fatFirstFree = CLUSTER_FIRST;

	while ((FAT_NextCluster(fatFirstFree) != CLUSTER_FREE) && (fatFirstFree <= fatLastCluster))
	{
		fatFirstFree++;
	}
	if (fatFirstFree > fatLastCluster)
	{
		return CLUSTER_EOF;
	}
	return fatFirstFree;
}

#ifdef CAN_WRITE_TO_DISC
/*-----------------------------------------------------------------
FAT_LinkFreeCluster
Internal function - gets the first available free cluster, sets it
to end of file, links the input cluster to it then returns the 
cluster number
-----------------------------------------------------------------*/
u32 FAT_LinkFreeCluster(u32 cluster)
{
	u32 firstFree;
	u32 curLink;

	if (cluster > fatLastCluster)
	{
		return CLUSTER_FREE;
	}

	// Check if the cluster already has a link, and return it if so
	curLink = FAT_NextCluster (cluster);
	if ((curLink >= CLUSTER_FIRST) && (curLink < fatLastCluster))
	{
		return curLink;	// Return the current link - don't allocate a new one
	}
	
	// Get a free cluster
	firstFree = FAT_FirstFreeCluster();

	// If couldn't get a free cluster then return
	if (firstFree == CLUSTER_EOF)
	{
		return CLUSTER_FREE;
	}

	if ((cluster >= CLUSTER_FIRST) && (cluster < fatLastCluster))
	{
		// Update the linked from FAT entry
		FAT_WriteFatEntry (cluster, firstFree);
	}
	// Create the linked to FAT entry
	FAT_WriteFatEntry (firstFree, CLUSTER_EOF);

	return firstFree;
}
#endif
/*-----------------------------------------------------------------
FAT_GetFilename
Get the alias (short name) of the file or directory stored in 
	dirEntry
DIR_ENT dirEntry: IN a valid directory table entry
char* alias OUT: will be filled with the alias (short filename),
	should be at least 13 bytes long
bool return OUT: return true if successful
-----------------------------------------------------------------*/
bool FAT_GetFilename (DIR_ENT dirEntry, char* alias)
{
	int i=0;
	int j=0;

	alias[0] = '\0';
	if (dirEntry.name[0] != FILE_FREE)
	{
		if (dirEntry.name[0] == '.')
		{
			alias[0] = '.';
			if (dirEntry.name[1] == '.')
			{
				alias[1] = '.';
				alias[2] = '\0';
			}
			else
			{
				alias[1] = '\0';
			}
		}
		else
		{		
			// Copy the filename from the dirEntry to the string
			for (i = 0; (i < 8) && (dirEntry.name[i] != ' '); i++)
			{
				alias[i] = dirEntry.name[i];
			}
			// Copy the extension from the dirEntry to the string
			if (dirEntry.ext[0] != ' ')
			{
				alias[i++] = '.';
				for ( j = 0; (j < 3) && (dirEntry.ext[j] != ' '); j++)
				{
					alias[i++] = dirEntry.ext[j];
				}
			}
			alias[i] = '\0';
		}
	}

	return (alias[0] != '\0');
}

/*-----------------------------------------------------------------
FAT_GetDirEntry
Return the file info structure of the next valid file entry
u32 dirCluster: IN cluster of subdirectory table
int entry: IN the desired file entry
int origin IN: relative position of the entry
DIR_ENT return OUT: desired dirEntry. First char will be FILE_FREE if 
	the entry does not exist.
-----------------------------------------------------------------*/
DIR_ENT FAT_GetDirEntry ( u32 dirCluster, int entry, int origin)
{
	DIR_ENT dir;
	DIR_ENT_LFN lfn;
	int firstSector = 0;
	bool notFound = false;
	bool found = false;
	int maxSectors;
	int lfnPos, aliasPos;
	u8 lfnChkSum, chkSum;

	dir.name[0] = FILE_FREE; // default to no file found
	dir.attrib = 0x00;

	// Check if fat has been initialised
	if (filesysBytePerSec == 0)
	{
		return (dir);
	}
	
	switch (origin) 
	{
	case SEEK_SET:
		wrkDirCluster = dirCluster;
		wrkDirSector = 0;
		wrkDirOffset = -1;
		break;
	case SEEK_CUR:	// Don't change anything
		break;
	case SEEK_END:	// Find entry signifying end of directory
		// Subtraction will never reach 0, so it keeps going 
		// until reaches end of directory
		wrkDirCluster = dirCluster;
		wrkDirSector = 0;
		wrkDirOffset = -1;
		entry = -1;
		break;
	default:
		return dir;
	}

	lfnChkSum = 0;
	maxSectors = (wrkDirCluster == FAT16_ROOT_DIR_CLUSTER ? (filesysData - filesysRootDir) : filesysSecPerClus);

	// Scan Dir for correct entry
	firstSector = (wrkDirCluster == FAT16_ROOT_DIR_CLUSTER ? filesysRootDir : FAT_ClustToSect(wrkDirCluster));
	if(wrkDirOffset==-1) disc_ReadSector (firstSector + wrkDirSector, globalBuffer);
	found = false;
	notFound = false;
	do {
		wrkDirOffset++;
		if (wrkDirOffset == BYTE_PER_READ / sizeof (DIR_ENT))
		{
			wrkDirOffset = 0;
			wrkDirSector++;
			if ((wrkDirSector == filesysSecPerClus) && (wrkDirCluster != FAT16_ROOT_DIR_CLUSTER))
			{
				wrkDirSector = 0;
				wrkDirCluster = FAT_NextCluster(wrkDirCluster);
				if (wrkDirCluster == CLUSTER_EOF)
				{
					notFound = true;
				}
				firstSector = FAT_ClustToSect(wrkDirCluster);		
			}
			else if ((wrkDirCluster == FAT16_ROOT_DIR_CLUSTER) && (wrkDirSector == (filesysData - filesysRootDir)))
			{
				notFound = true;	// Got to end of root dir
			}
			disc_ReadSector (firstSector + wrkDirSector, globalBuffer);
		}
		dir = ((DIR_ENT*) globalBuffer)[wrkDirOffset];
		if ((dir.name[0] != FILE_FREE) && (dir.name[0] > 0x20) && ((dir.attrib & ATTRIB_VOL) != ATTRIB_VOL))
		{
			entry--;
			if (lfnExists)
			{
				// Calculate file checksum
				chkSum = 0;
				for (aliasPos=0; aliasPos < 11; aliasPos++)
				{
					// NOTE: The operation is an unsigned char rotate right
					chkSum = ((chkSum & 1) ? 0x80 : 0) + (chkSum >> 1) + (aliasPos < 8 ? dir.name[aliasPos] : dir.ext[aliasPos - 8]);
				}
				if (chkSum != lfnChkSum)
				{
					lfnExists = false;
					lfnName[0] = '\0';
// #include "gba_nds_fat_chg/getdirentry1.h"
					lfnNameUnicode[0] = 0;
				}
			}
			if (entry == 0) 
			{
				if (!lfnExists)
				{
					FAT_GetFilename (dir, lfnName);
					//#include "gba_nds_fat_chg/getdirentry2.h"
					bool NTF_lowfn=false,NTF_lowext=false;
					if((dir.reserved&BIT(3))!=0) NTF_lowfn=true;
					if((dir.reserved&BIT(4))!=0) NTF_lowext=true;

					if((NTF_lowfn==false)&&(NTF_lowext==false)){
						u32 idx;
						for(idx=0;idx<MAX_FILENAME_LENGTH;idx++){
							char fc=lfnName[idx];
							lfnNameUnicode[idx]=fc;
							if(fc==0) break;
						}
					}else{
						u32 posperiod=(u32)-1;
						{
							u32 idx;
							for(idx=0;idx<MAX_FILENAME_LENGTH;idx++){
								char fc=lfnName[idx];
								if(fc=='.') posperiod=idx;
								if(fc==0) break;
							}
						}
						if(posperiod==(u32)-1){
							u32 idx;
							for(idx=0;idx<MAX_FILENAME_LENGTH;idx++){
								char fc=lfnName[idx];
								if(NTF_lowfn==true){
									if(('A'<=fc)&&(fc<='Z')) fc+=0x20;
								}
								lfnNameUnicode[idx]=fc;
								if(fc==0) break;
							}
						}else{
							u32 idx;
							for(idx=0;idx<MAX_FILENAME_LENGTH;idx++){
								char fc=lfnName[idx];
								if(NTF_lowfn==true){
									if(('A'<=fc)&&(fc<='Z')) fc+=0x20;
								}
								lfnNameUnicode[idx]=fc;
								if(fc=='.') break;
							}
							for(;idx<MAX_FILENAME_LENGTH;idx++){
								char fc=lfnName[idx];
								if(NTF_lowext==true){
									if(('A'<=fc)&&(fc<='Z')) fc+=0x20;
								}
								lfnNameUnicode[idx]=fc;
								if(fc==0) break;
							}
						}
					}
				}
				found = true;
			}
		}
		else if (dir.name[0] == FILE_LAST)
		{
			if (origin == SEEK_END)
			{
				found = true;
			}
			else
			{
				notFound = true;
			}
		}
		else if (dir.attrib == ATTRIB_LFN)
		{
			lfn = ((DIR_ENT_LFN*) globalBuffer)[wrkDirOffset];
			if (lfn.ordinal & LFN_DEL)
			{
				lfnExists = false;
			}
			else if (lfn.ordinal & LFN_END)	// Last part of LFN, make sure it isn't deleted (Thanks MoonLight)
			{
				lfnExists = true;
				lfnName[(lfn.ordinal & ~LFN_END) * 13] = '\0';	// Set end of lfn to null character
//#include "gba_nds_fat_chg/getdirentry3.h"
				lfnNameUnicode[(lfn.ordinal & ~LFN_END) * 13] = 0;	// Set end of lfn to null character
				lfnChkSum = lfn.checkSum;
			}
			if (lfnChkSum != lfn.checkSum)
			{
				lfnExists = false;
			}
			if (lfnExists)
			{
				lfnPos = ((lfn.ordinal & ~LFN_END) - 1) * 13;
				lfnName[lfnPos + 0] = lfn.char0 & 0xFF;
				lfnName[lfnPos + 1] = lfn.char1 & 0xFF;
				lfnName[lfnPos + 2] = lfn.char2 & 0xFF;
				lfnName[lfnPos + 3] = lfn.char3 & 0xFF;
				lfnName[lfnPos + 4] = lfn.char4 & 0xFF;
				lfnName[lfnPos + 5] = lfn.char5 & 0xFF;
				lfnName[lfnPos + 6] = lfn.char6 & 0xFF;
				lfnName[lfnPos + 7] = lfn.char7 & 0xFF;
				lfnName[lfnPos + 8] = lfn.char8 & 0xFF;
				lfnName[lfnPos + 9] = lfn.char9 & 0xFF;
				lfnName[lfnPos + 10] = lfn.char10 & 0xFF;
				lfnName[lfnPos + 11] = lfn.char11 & 0xFF;
				lfnName[lfnPos + 12] = lfn.char12 & 0xFF;
//#include "gba_nds_fat_chg/getdirentry4.h"
				lfnNameUnicode[lfnPos + 0] = lfn.char0;
				lfnNameUnicode[lfnPos + 1] = lfn.char1;
				lfnNameUnicode[lfnPos + 2] = lfn.char2;
				lfnNameUnicode[lfnPos + 3] = lfn.char3;
				lfnNameUnicode[lfnPos + 4] = lfn.char4;
				lfnNameUnicode[lfnPos + 5] = lfn.char5;
				lfnNameUnicode[lfnPos + 6] = lfn.char6;
				lfnNameUnicode[lfnPos + 7] = lfn.char7;
				lfnNameUnicode[lfnPos + 8] = lfn.char8;
				lfnNameUnicode[lfnPos + 9] = lfn.char9;
				lfnNameUnicode[lfnPos + 10] = lfn.char10;
				lfnNameUnicode[lfnPos + 11] = lfn.char11;
				lfnNameUnicode[lfnPos + 12] = lfn.char12;
			}
		}
	} while (!found && !notFound);
	
	// If no file is found, return FILE_FREE
	if (notFound)
	{
		dir.name[0] = FILE_FREE;
	}

	return (dir);
}



/*-----------------------------------------------------------------
FAT_DirEntFromPath
Finds the directory entry for a file or directory from a path
Path separator is a forward slash /
const char* path: IN null terminated string of path.
DIR_ENT return OUT: dirEntry of found file. First char will be FILE_FREE
	if the file was not found
-----------------------------------------------------------------*/
DIR_ENT FAT_DirEntFromPath (const char* path)
{
	int pathPos;
	char name[MAX_FILENAME_LENGTH];
	char alias[13];
	int namePos;
	bool found, notFound;
	DIR_ENT dirEntry;
	u32 dirCluster;
	bool flagLFN, dotSeen;
	
	// Start at beginning of path
	pathPos = 0;
	
	if (path[pathPos] == '/') 
	{
		dirCluster = filesysRootDirClus;	// Start at root directory
	}
	else
	{
		dirCluster = curWorkDirCluster;	// Start at current working dir
	}
	
	// Eat any slash /
	while ((path[pathPos] == '/') && (path[pathPos] != '\0'))
	{
		pathPos++;
	}
	
	// Search until can't continue
	found = false;
	notFound = false;
	while (!notFound && !found)
	{
		flagLFN = false;
		// Copy name from path
		namePos = 0;
		if ((path[pathPos] == '.') && ((path[pathPos + 1] == '\0') || (path[pathPos + 1] == '/'))) {
			// Dot entry
			name[namePos++] = '.';
			pathPos++;
		} else if ((path[pathPos] == '.') && (path[pathPos + 1] == '.') && ((path[pathPos + 2] == '\0') || (path[pathPos + 2] == '/'))){
			// Double dot entry
			name[namePos++] = '.';
			pathPos++;
			name[namePos++] = '.';
			pathPos++;
		} else {
			// Copy name from path
			if (path[pathPos] == '.') {
				flagLFN = true;
			}
			dotSeen = false;
			while ((namePos < MAX_FILENAME_LENGTH - 1) && (path[pathPos] != '\0') && (path[pathPos] != '/'))
			{
				name[namePos] = ucase(path[pathPos]);
				if ((name[namePos] <= ' ') || ((name[namePos] >= ':') && (name[namePos] <= '?'))) // Invalid character
				{
					flagLFN = true;
				}
				if (name[namePos] == '.') {
					if (!dotSeen) {
						dotSeen = true;
					} else {
						flagLFN = true;
					}
				}
				namePos++;
				pathPos++;
			}
			// Check if a long filename was specified
			if (namePos > 12)
			{
				flagLFN = true;
			}
		}
		
		// Add end of string char
		name[namePos] = '\0';

		// Move through path to correct place
		while ((path[pathPos] != '/') && (path[pathPos] != '\0'))
			pathPos++;
		// Eat any slash /
		while ((path[pathPos] == '/') && (path[pathPos] != '\0'))
		{
			pathPos++;
		}

		// Search current Dir for correct entry
		dirEntry = FAT_GetDirEntry (dirCluster, 1, SEEK_SET);
		while ( !found && !notFound)
		{
			// Match filename
			found = true;
			for (namePos = 0; (namePos < MAX_FILENAME_LENGTH) && found && (name[namePos] != '\0') && (lfnName[namePos] != '\0'); namePos++)
			{
				if (name[namePos] != ucase(lfnName[namePos]))
				{
					found = false;
				}
			}
			if ((name[namePos] == '\0') != (lfnName[namePos] == '\0'))
			{
				found = false;
			}

			// Check against alias as well.
			if (!found)
			{
				FAT_GetFilename(dirEntry, alias);
				found = true;
				for (namePos = 0; (namePos < 13) && found && (name[namePos] != '\0') && (alias[namePos] != '\0'); namePos++)
				{
					if (name[namePos] != ucase(alias[namePos]))
					{
						found = false;
					}
				}
				if ((name[namePos] == '\0') != (alias[namePos] == '\0'))
				{
					found = false;
				}
			}

			if (dirEntry.name[0] == FILE_FREE)
				// Couldn't find specified file
			{
				found = false;
				notFound = true;
			}
			if (!found && !notFound)
			{
				dirEntry = FAT_GetDirEntry (dirCluster, 1, SEEK_CUR);
			}
		}
		
		if (found && ((dirEntry.attrib & ATTRIB_DIR) == ATTRIB_DIR) && (path[pathPos] != '\0'))
			// It has found a directory from within the path that needs to be followed
		{
			found = false;
			dirCluster = dirEntry.startCluster | (dirEntry.startClusterHigh << 16);
		}
	}
	
	if (notFound)
	{
		dirEntry.name[0] = FILE_FREE;
		dirEntry.attrib = 0x00;
	}

	return (dirEntry);
}

/*-----------------------------------------------------------------
FAT_fopen(filename, mode)
Opens a file
const char* path: IN null terminated string of filename and path 
	separated by forward slashes, / is root
const char* mode: IN mode to open file in
	Supported modes: "r", "r+", "w", "w+", "a", "a+", don't use
	"b" or "t" in any mode, as all files are openned in binary mode
FAT_FILE* return: OUT handle to open file, returns NULL if the file 
	couldn't be openned
-----------------------------------------------------------------*/
FAT_FILE* FAT_fopen(const char* path, const char* mode)
{
	int fileNum;
	FAT_FILE* file;
	DIR_ENT dirEntry;
#ifdef CAN_WRITE_TO_DISC
	u32 startCluster;
	int clusCount;
#endif

	const char* pchTemp;
	// Check that a valid mode was specified
	pchTemp = strpbrk ( mode, "rRwWaA" );
	if (pchTemp == NULL)
	{
		return NULL;
	}
	if (strpbrk ( pchTemp+1, "rRwWaA" ) != NULL)
	{
		return NULL;
	}
		
	// Get the dirEntry for the path specified
	dirEntry = FAT_DirEntFromPath (path);
	
	// Check that it is not a directory
	if (dirEntry.attrib & ATTRIB_DIR)
	{
		return NULL;
	}

#ifdef CAN_WRITE_TO_DISC
	// Check that it is not a read only file being openned in a writing mode
	if ( (strpbrk(mode, "wWaA+") != NULL) && (dirEntry.attrib & ATTRIB_RO))
	{
		return NULL;
	}
#else
	if ( (strpbrk(mode, "wWaA+") != NULL))
	{
		return NULL;
	}
#endif

	// Find a free file buffer
	for (fileNum = 0; (fileNum < MAX_FILES_OPEN) && (openFiles[fileNum].inUse == true); fileNum++);
	
	if (fileNum == MAX_FILES_OPEN) // No free files
	{
		return NULL;
	}

	file = &openFiles[fileNum];
	// Remember where directory entry was
	file->dirEntSector = (wrkDirCluster == FAT16_ROOT_DIR_CLUSTER ? filesysRootDir : FAT_ClustToSect(wrkDirCluster)) + wrkDirSector;
	file->dirEntOffset = wrkDirOffset;

	if ( strpbrk(mode, "rR") != NULL )  //(ucase(mode[0]) == 'R')
	{
		if (dirEntry.name[0] == FILE_FREE)	// File must exist
		{
			return NULL;
		}
		
		file->read = true;
#ifdef CAN_WRITE_TO_DISC
		file->write = ( strchr(mode, '+') != NULL ); //(mode[1] == '+');
#else
		file->write = false;
#endif
		file->append = false;
		
		// Store information about position within the file, for use
		// by FAT_fread, FAT_fseek, etc.
		file->firstCluster = dirEntry.startCluster | (dirEntry.startClusterHigh << 16);
	
#ifdef CAN_WRITE_TO_DISC
		// Check if file is openned for random. If it is, and currently has no cluster, one must be 
		// assigned to it.
		if (file->write && file->firstCluster == CLUSTER_FREE)
		{
			file->firstCluster = FAT_LinkFreeCluster (CLUSTER_FREE);
			if (file->firstCluster == CLUSTER_FREE)	// Couldn't get a free cluster
			{
				return NULL;
			}

			// Store cluster position into the directory entry
			dirEntry.startCluster = (file->firstCluster & 0xFFFF);
			dirEntry.startClusterHigh = ((file->firstCluster >> 16) & 0xFFFF);
			disc_ReadSector (file->dirEntSector, globalBuffer);
			((DIR_ENT*) globalBuffer)[file->dirEntOffset] = dirEntry;
			disc_WriteSector (file->dirEntSector, globalBuffer);
		}
#endif
			
		file->length = dirEntry.fileSize;
		file->curPos = 0;
		file->curClus = dirEntry.startCluster | (dirEntry.startClusterHigh << 16);
		file->curSect = 0;
		file->curByte = 0;

		// Not appending
		file->appByte = 0;
		file->appClus = 0;
		file->appSect = 0;
	
		disc_ReadSector( FAT_ClustToSect( file->curClus), file->readBuffer);
		file->inUse = true;	// We're using this file now

		return file;
	}	// mode "r"

#ifdef CAN_WRITE_TO_DISC
	if ( strpbrk(mode, "wW") != NULL ) // (ucase(mode[0]) == 'W')
	{
		if (dirEntry.name[0] == FILE_FREE)	// Create file if it doesn't exist
		{
			dirEntry.attrib = ATTRIB_ARCH;
			dirEntry.reserved = 0;
			
			// Time and date set to system time and date
			dirEntry.cTime_ms = 0;
			dirEntry.cTime = getRTCtoFileTime();
			dirEntry.cDate = getRTCtoFileDate();
			dirEntry.aDate = getRTCtoFileDate();
			dirEntry.mTime = getRTCtoFileTime();
			dirEntry.mDate = getRTCtoFileDate();
		}
		else	// Already a file entry 
		{
			// Free any clusters used
			FAT_ClearLinks (dirEntry.startCluster | (dirEntry.startClusterHigh << 16));
		}
		
		// Get a cluster to use
		startCluster = FAT_LinkFreeCluster (CLUSTER_FREE);
		if (startCluster == CLUSTER_FREE)	// Couldn't get a free cluster
		{
			return NULL;
		}

		// Store cluster position into the directory entry
		dirEntry.startCluster = (startCluster & 0xFFFF);
		dirEntry.startClusterHigh = ((startCluster >> 16) & 0xFFFF);

		// The file has no data in it - its over written so should be empty
		dirEntry.fileSize = 0;

		if (dirEntry.name[0] == FILE_FREE)	// No file
		{
			// Have to create a new entry
			if(!FAT_AddDirEntry (path, dirEntry))
			{
				return NULL;
			}
			// Get the newly created dirEntry
			dirEntry = FAT_DirEntFromPath (path);

			// Remember where directory entry was
			file->dirEntSector = (wrkDirCluster == FAT16_ROOT_DIR_CLUSTER ? filesysRootDir : FAT_ClustToSect(wrkDirCluster)) + wrkDirSector;
			file->dirEntOffset = wrkDirOffset;
		}
		else	// Already a file
		{
			// Just modify the old entry
			disc_ReadSector (file->dirEntSector, globalBuffer);
			((DIR_ENT*) globalBuffer)[file->dirEntOffset] = dirEntry;
			disc_WriteSector (file->dirEntSector, globalBuffer);
		}
		

		// Now that file is created, open it
		file->read = ( strchr(mode, '+') != NULL ); //(mode[1] == '+');
		file->write = true;
		file->append = false;
		
		// Store information about position within the file, for use
		// by FAT_fread, FAT_fseek, etc.
		file->firstCluster = startCluster;
		file->length = 0;	// Should always have 0 bytes if openning in "w" mode
		file->curPos = 0;
		file->curClus = startCluster;
		file->curSect = 0;
		file->curByte = 0;

		// Not appending
		file->appByte = 0;
		file->appClus = 0;
		file->appSect = 0;
		
		// Empty file, so empty read buffer
		memset (file->readBuffer, 0, BYTE_PER_READ);
		file->inUse = true;	// We're using this file now

		return file;
	}

	if ( strpbrk(mode, "aA") != NULL ) // (ucase(mode[0]) == 'A')
	{
		if (dirEntry.name[0] == FILE_FREE)	// Create file if it doesn't exist
		{
			dirEntry.attrib = ATTRIB_ARCH;
			dirEntry.reserved = 0;
			
			// Time and date set to system time and date
			dirEntry.cTime_ms = 0;
			dirEntry.cTime = getRTCtoFileTime();
			dirEntry.cDate = getRTCtoFileDate();
			dirEntry.aDate = getRTCtoFileDate();
			dirEntry.mTime = getRTCtoFileTime();
			dirEntry.mDate = getRTCtoFileDate();

			// The file has no data in it
			dirEntry.fileSize = 0;

			// Get a cluster to use
			startCluster = FAT_LinkFreeCluster (CLUSTER_FREE);
			if (startCluster == CLUSTER_FREE)	// Couldn't get a free cluster
			{
				return NULL;
			}
			dirEntry.startCluster = (startCluster & 0xFFFF);
			dirEntry.startClusterHigh = ((startCluster >> 16) & 0xFFFF);
			
			if(!FAT_AddDirEntry (path, dirEntry))
				return NULL;
			
			// Get the newly created dirEntry
			dirEntry = FAT_DirEntFromPath (path);
			
			// Store append cluster
			file->appClus = startCluster;

			// Remember where directory entry was
			file->dirEntSector = (wrkDirCluster == FAT16_ROOT_DIR_CLUSTER ? filesysRootDir : FAT_ClustToSect(wrkDirCluster)) + wrkDirSector;
			file->dirEntOffset = wrkDirOffset;
		}
		else	// File already exists - reuse the old directory entry
		{
			startCluster = dirEntry.startCluster | (dirEntry.startClusterHigh << 16);
			// If it currently has no cluster, one must be assigned to it.
			if (startCluster == CLUSTER_FREE)
			{
				file->firstCluster = FAT_LinkFreeCluster (CLUSTER_FREE);
				if (file->firstCluster == CLUSTER_FREE)	// Couldn't get a free cluster
				{
					return NULL;
				}
				
				// Store cluster position into the directory entry
				dirEntry.startCluster = (file->firstCluster & 0xFFFF);
				dirEntry.startClusterHigh = ((file->firstCluster >> 16) & 0xFFFF);
				disc_ReadSector (file->dirEntSector, globalBuffer);
				((DIR_ENT*) globalBuffer)[file->dirEntOffset] = dirEntry;
				disc_WriteSector (file->dirEntSector, globalBuffer);

				// Store append cluster
				file->appClus = startCluster;
		
			} else {

				// Follow cluster list until last one is found
				clusCount = dirEntry.fileSize / filesysBytePerClus;
				file->appClus = startCluster;
				while ((clusCount--) && (FAT_NextCluster (file->appClus) != CLUSTER_FREE) && (FAT_NextCluster (file->appClus) != CLUSTER_EOF))
				{
					file->appClus = FAT_NextCluster (file->appClus);
				}
				if (clusCount >= 0) // Check if ran out of clusters
				{
					// Set flag to allocate new cluster when needed
					file->appSect = filesysSecPerClus;
					file->appByte = 0;
				}
			}
		}

		// Now that file is created, open it
		file->read = ( strchr(mode, '+') != NULL );
		file->write = false;
		file->append = true;
		
		// Calculate the sector and byte of the current position,
		// and store them
		file->appSect = (dirEntry.fileSize % filesysBytePerClus) / BYTE_PER_READ;
		file->appByte = dirEntry.fileSize % BYTE_PER_READ;

		// Store information about position within the file, for use
		// by FAT_fread, FAT_fseek, etc.
		file->firstCluster = startCluster;
		file->length = dirEntry.fileSize;
		file->curPos = dirEntry.fileSize;
		file->curClus = file->appClus;
		file->curSect = file->appSect;
		file->curByte = file->appByte;
		
		// Read into buffer
		disc_ReadSector( FAT_ClustToSect(file->curClus) + file->curSect, file->readBuffer);
		file->inUse = true;	// We're using this file now
		return file;
	}
#endif

	// Can only reach here if a bad mode was specified
	return NULL;
}
