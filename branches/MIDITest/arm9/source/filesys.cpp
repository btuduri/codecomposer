#include <nds.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "filesys.h"
#include "inifile.h"

#include "strtool.h"
#include "_const.h"

#include "unicode.h"

#define FAT_FT_END (0)
#define FAT_FT_FILE (1)
#define FAT_FT_DIR (2)

char FileSys_PathName[PathnameMaxLength]="/";

typedef struct {
  u32 FileType;
  u32 FileSize;
  char Alias[AliasMaxLength];
  UnicodeChar Filename[FilenameMaxLength];
  int TrackNum;
} TFile;

static int FileMaxCount;

static TFile *pFile=NULL;
static s32 FileCount=0;

char *FileSys_InitInterface_ErrorStr=NULL;

static char LastPathItemName[PathnameMaxLength];

static int ShuffleCount=-1;
static int *pShuffleList=NULL;

extern void _fatchk(char *file,u32 line);
#define fatchk() { _fatchk(__FILE__,__LINE__); }

void FileSys_Init(int _FileMaxCount)
{ 
  FileSys_Free();
  
  FileMaxCount=_FileMaxCount;
  pFile=(TFile*)malloc(FileMaxCount*sizeof(TFile));
  pShuffleList=(int*)malloc(FileMaxCount*4);
  
  LastPathItemName[0]=0;
  
  iprintf("FileMaxCount=%d\n",FileMaxCount);
}

void FileSys_Free(void)
{
  if(pFile!=NULL){
    free(pFile); pFile=NULL;
  }
  
  if(pShuffleList!=NULL){
    free(pShuffleList); pShuffleList=NULL;
  }
  
  FileCount=0;
  strcpy(FileSys_PathName,"/");
}

static bool InitFlag_MPCF=false;

bool FileSys_InitInterface(void)
{ 
  FileSys_InitInterface_ErrorStr="";
  
  bool res=false;
  
  SetARM9_REG_WaitCR();
  
  if(InitFlag_MPCF==true){
    res=true;
    }else{
    res=InitFlag_MPCF=FAT_InitFiles();
    if(res==false) FileSys_InitInterface_ErrorStr="media not insert or not found error.";
  }
  
  SetARM9_REG_WaitCR();
  
  return(res);
}

void FileSys_FreeInterface(void)
{ 
  SetARM9_REG_WaitCR();
  
  if(InitFlag_MPCF==true){
  }
  
  SetARM9_REG_WaitCR();
}

static void FileSys_RefreshPath_DuplicateTracks(int tracks)
{
  if(FileCount==FileMaxCount) return;
  
  u32 SrcFileIndex=FileCount-1;
  
  pFile[SrcFileIndex].TrackNum=0;
  int idx=1;
  
  for(idx=1;idx<tracks;idx++){
    pFile[FileCount].FileType=pFile[SrcFileIndex].FileType;
    pFile[FileCount].FileSize=pFile[SrcFileIndex].FileSize;
    {
      char *src=pFile[SrcFileIndex].Alias;
      char *dst=pFile[FileCount].Alias;
      while(*src!=0){
        *dst++=*src++;
      }
      *dst=0;
    }
    {
      UnicodeChar *src=pFile[SrcFileIndex].Filename;
      UnicodeChar *dst=pFile[FileCount].Filename;
      while(*src!=0){
        *dst++=*src++;
      }
      *dst=0;
    }
    pFile[FileCount].TrackNum=idx;
    FileCount++;
    if(FileCount==FileMaxCount) break;
  }
}

static void FileSys_RefreshPath_GMENSF_DuplicateTracks(int FileHandle)
{
  int tracks=-1;
  
  u8 buf[7];
  
  if(FAT_fread(&buf[0],1,7,(FAT_FILE*)FileHandle)==7){
    if((buf[0]=='N')&&(buf[1]=='E')&&(buf[2]=='S')&&(buf[3]=='M')&&(buf[4]==0x1a)){
      tracks=buf[6];
    }
  }
  
  if(tracks!=-1) FileSys_RefreshPath_DuplicateTracks(tracks);
}

static void FileSys_RefreshPath_GMEGBS_DuplicateTracks(int FileHandle)
{
  int tracks=-1;
  
  u8 buf[5];
  
  if(FAT_fread(&buf[0],1,5,(FAT_FILE*)FileHandle)==5){
    if((buf[0]=='G')&&(buf[1]=='B')&&(buf[2]=='S')){
      if(buf[3]==1){
        tracks=buf[4];
      }
    }
  }
  
  if(tracks!=-1) FileSys_RefreshPath_DuplicateTracks(tracks);
}

/*
// There are two MIDIINI; shit how does it make sense! - KHS

static void FileSys_RefreshPath_GMEHES_DuplicateTracks(void)
{
  FileSys_RefreshPath_DuplicateTracks(MIDIINI.GMEPlugin.HES_MaxTrackNumber);
}
*/

static void FileSys_RefreshPath_GMESAP_DuplicateTracks(int FileHandle)
{
  int tracks=-1;
  
  FAT_fseek((FAT_FILE*)FileHandle,0,SEEK_END);
  u32 size=FAT_ftell((FAT_FILE*)FileHandle);
  FAT_fseek((FAT_FILE*)FileHandle,0,SEEK_SET);
  
  if(size<16){
    iprintf("DataSize:%d<%d\n",size,16);
    return;
  }
  
  u8 buf[256];
  u32 idx=0;
  
  for(idx=0;idx<256;idx++) buf[idx]=0;
  
  FAT_fread(&buf[0],1,256,(FAT_FILE*)FileHandle);
  
#define writebyte(pos,data) (buf[pos]=data)
#define readbyte(pos) (buf[pos])
#define readword(pos) (((u16)readbyte(pos) << 8)|((u16)readbyte(pos+1) << 0))
#define readdword(pos) (((u32)readbyte(pos) << 24)|((u32)readbyte(pos+1) << 16)|((u32)readbyte(pos+2) << 8)|((u32)readbyte(pos+3) << 0))
#define readstr(pos) (&buf[pos])

  const char ID[]="SAP\x0D\x0A";
  
  for(idx=0;idx<5;idx++){
    if(readbyte(idx)!=ID[idx]){
      iprintf("Unknown format.\n");
      return;
    }
  }
  
  bool isSection;
  char Section[128],Value[128];
  int SectionCount,ValueCount;
  u32 pos=0;
  
  isSection=true;
  Section[0]=0;
  SectionCount=0;
  Value[0]=0;
  ValueCount=0;
  
  while(1){
    char c0=readbyte(pos+0),c1=readbyte(pos+1);
    if((c0==0xff)&&(c1==0xff)) break;
    
    if((c0!=0x0d)||(c1!=0x0a)){
      pos+=1;
      if(c0==' '){
        isSection=false;
        }else{
        if(isSection==true){
          Section[SectionCount++]=c0;
          }else{
          Value[ValueCount++]=c0;
        }
      }
      }else{
      pos+=2;
      Section[SectionCount]=0;
      Value[ValueCount]=0;
      
      if(strcmp(Section,"SONGS")==0){
        tracks=0;
        char *psrc=Value;
        if(*psrc!='\"'){
          while(*psrc!=0){
            tracks<<=4;
            char ch=*psrc++;
            if(('0'<=ch)&&(ch<='9')) tracks+=((u32)ch)-((u32)'0');
            if(('a'<=ch)&&(ch<='f')) tracks+=0x10+(((u32)ch)-((u32)'a'));
            if(('A'<=ch)&&(ch<='F')) tracks+=0x10+(((u32)ch)-((u32)'A'));
          }
        }
      }
      
      isSection=true;
      Section[0]=0;
      SectionCount=0;
      Value[0]=0;
      ValueCount=0;
    }
  }
  
#undef writebyte
#undef readbyte
#undef readword
#undef readdword
#undef readstr
  
  if(tracks!=-1) FileSys_RefreshPath_DuplicateTracks(tracks);
}

static void FileSys_RefreshPath_GMEKSS_DuplicateTracks(int FileHandle)
{
  int tracks=-1;
  
  u8 buf[0x20];
  
  if(FAT_fread(&buf[0],1,0x20,(FAT_FILE*)FileHandle)==0x20){
    if((buf[0]=='K')&&(buf[1]=='S')&&(buf[2]=='S')&&(buf[3]=='X')){
      if(buf[0x0e]==0x10){
        tracks=((u16)buf[0x1a]<<0)|((u16)buf[0x1b]<<8);
      }
    }
  }
  
  if(tracks!=-1) FileSys_RefreshPath_DuplicateTracks(tracks);
}

static void FileSys_RefreshPath_GMEAY_DuplicateTracks(int FileHandle)
{
  int tracks=-1;
  
  u8 buf[0x11];
  
  if(FAT_fread(&buf[0],1,0x11,(FAT_FILE*)FileHandle)==0x11){
    if((buf[0]=='Z')&&(buf[1]=='X')&&(buf[2]=='A')&&(buf[3]=='Y')&&(buf[4]=='E')&&(buf[5]=='M')&&(buf[6]=='U')&&(buf[7]=='L')){
      tracks=buf[0x10]+1;
    }
  }
  
  if(tracks!=-1) FileSys_RefreshPath_DuplicateTracks(tracks);
}

void FileSys_RefreshPath(void)
{ 
  int idx=0;
  for(idx=0;idx<FileMaxCount;idx++){
    TFile *_pFile=&pFile[idx];
    _pFile->FileType=0;
    _pFile->FileSize=0;
    _pFile->Alias[0]=0;
    _pFile->Filename[0]=0;
    _pFile->TrackNum=-1;
  }
  
  if(FileSys_InitInterface()==false){ 
    iprintf("FileSys_InitInterface()==false; fatal error.\n");
  }
  
  bool InsertDoubleDotPath;
  
  if(FileSys_PathName[0]==0){ 
    strcpy(FileSys_PathName,"/");
    }else{
    if((FileSys_PathName[0]=='/')&&(FileSys_PathName[1]==0)){ 
      strcpy(FileSys_PathName,"/");
      }else{
      if(FileSys_PathName[0]!='/'){ 
        strcpy(FileSys_PathName,"/");
      }
    }
  }
  
  if((FileSys_PathName[0]=='/')&&(FileSys_PathName[1]==0)){ 
    FAT_CWD("/");
    InsertDoubleDotPath=false;
    }else{
    FAT_CWD(FileSys_PathName);
    InsertDoubleDotPath=true;
  }
  
  char fn[FilenameMaxLength];
  u32 FAT_FileType;
  
  FileCount=0;
  
  if(InsertDoubleDotPath==true){ 
    pFile[FileCount].FileType=FT_Path;
    pFile[FileCount].FileSize=0;
    strcpy(pFile[FileCount].Alias,"..");
    StrConvert_Local2Unicode("../",pFile[FileCount].Filename);
    FileCount++;
  }
  
  FAT_FileType=FAT_FindFirstFile(fn);
  
  TiniHiddenItem *pHiddenItem = &GlobalINI.HiddenItem;

  u8 ReqHiddenAttrib=0;
  
  {
    const u8 ATTRIB_ARCH=0x20;
    const u8 ATTRIB_HID=0x02;
    const u8 ATTRIB_SYS=0x04;
    const u8 ATTRIB_RO=0x01;
    
    if(pHiddenItem->Attribute_Archive==true) ReqHiddenAttrib|=ATTRIB_ARCH;
    if(pHiddenItem->Attribute_Hidden==true) ReqHiddenAttrib|=ATTRIB_HID;
    if(pHiddenItem->Attribute_System==true) ReqHiddenAttrib|=ATTRIB_SYS;
    if(pHiddenItem->Attribute_Readonly==true) ReqHiddenAttrib|=ATTRIB_RO;
  }
  
  while(FAT_FileType!=FAT_FT_END){ 
    bool useflag=true;
    
    if(FAT_FileType==FAT_FT_DIR){
      if(strcmp(fn,".")==0) useflag=false;
      if(strcmp(fn,"..")==0) useflag=false;
      if((pHiddenItem->Path_Shell==true)&&(strcmp(fn,"SHELL")==0)) useflag=false;
      if((pHiddenItem->Path_Moonshl==true)&&(strcmp(fn,"MOONSHL")==0)) useflag=false;
    }
    if(FAT_FileType==FAT_FT_FILE){
      if((pHiddenItem->File_Thumbnail==true)&&(strcmp(fn,"_THUMBNL.MSL")==0)) useflag=false;
    }
    
    if(ReqHiddenAttrib!=0){
      if((FAT_GetAttrib()&ReqHiddenAttrib)!=0) useflag=false;
    }
    
    if(useflag==true){ 
      strcpy(pFile[FileCount].Alias,fn);
      
      char *PathChar="";
      
      switch(FAT_FileType){ 
        case FAT_FT_DIR:
          pFile[FileCount].FileType=FT_Path;
          pFile[FileCount].FileSize=0;
          PathChar="/";
          break;
        case FAT_FT_FILE:
          pFile[FileCount].FileType=FT_File;
          pFile[FileCount].FileSize=FAT_GetFileSize();
          PathChar=""; // 0x00
          break;
      }
      
      UnicodeChar *lfn=pFile[FileCount].Filename;
      
      if(FAT_GetLongFilenameUnicode(lfn,FilenameMaxLength)==false){ 
        StrConvert_Local2Unicode(fn,lfn);
      }
      
      u32 idx=0;
      while(1){ 
        if(lfn[idx]==0){ 
          StrConvert_Local2Unicode(PathChar,&lfn[idx]);
          break;
        }
        idx++;
      }
      
      FileCount++;
      if(FileCount==FileMaxCount) break;
      
      { // DuplicateTracks
        char *Alias=pFile[FileCount-1].Alias;
        int AliasLen=strlen(Alias);
        int exf=0;
        
        if(Alias[AliasLen-4]=='.'){
          if((Alias[AliasLen-3]=='N')&&(Alias[AliasLen-2]=='S')&&(Alias[AliasLen-1]=='F')) exf=1;
          if((Alias[AliasLen-3]=='G')&&(Alias[AliasLen-2]=='B')&&(Alias[AliasLen-1]=='S')) exf=2;
          if((Alias[AliasLen-3]=='H')&&(Alias[AliasLen-2]=='E')&&(Alias[AliasLen-1]=='S')) exf=3;
          if((Alias[AliasLen-3]=='S')&&(Alias[AliasLen-2]=='A')&&(Alias[AliasLen-1]=='P')) exf=5;
          if((Alias[AliasLen-3]=='K')&&(Alias[AliasLen-2]=='S')&&(Alias[AliasLen-1]=='S')) exf=6;
        }
        if(Alias[AliasLen-3]=='.'){
          if((Alias[AliasLen-2]=='A')&&(Alias[AliasLen-1]=='Y')) exf=4;
        }
        
        switch(exf){
          case 0: { // none
          } break;
          case 1: { // NSF
            int FileHandle=(int)FAT_fopen(Alias,"rb");
            
            if(FileHandle!=0){
              FileSys_RefreshPath_GMENSF_DuplicateTracks(FileHandle);
              FAT_fclose((FAT_FILE*)FileHandle);
            }
          } break;
          case 2: { // GBS
            int FileHandle=(int)FAT_fopen(Alias,"rb");
            
            if(FileHandle!=0){
              FileSys_RefreshPath_GMEGBS_DuplicateTracks(FileHandle);
              FAT_fclose((FAT_FILE*)FileHandle);
            }
          } break;
          case 3: { // HES
			/*
            FileSys_RefreshPath_GMEHES_DuplicateTracks();
			*/
          } break;
          case 4: { // AY
            int FileHandle=(int)FAT_fopen(Alias,"rb");
            
            if(FileHandle!=0){
              FileSys_RefreshPath_GMEAY_DuplicateTracks(FileHandle);
              FAT_fclose((FAT_FILE*)FileHandle);
            }
          } break;
          case 5: { // SAP
            int FileHandle=(int)FAT_fopen(Alias,"rb");
            
            if(FileHandle!=0){
              FileSys_RefreshPath_GMESAP_DuplicateTracks(FileHandle);
              FAT_fclose((FAT_FILE*)FileHandle);
            }
          } break;
          case 6: { // KSS
            int FileHandle=(int)FAT_fopen(Alias,"rb");
            
            if(FileHandle!=0){
              FileSys_RefreshPath_GMEKSS_DuplicateTracks(FileHandle);
              FAT_fclose((FAT_FILE*)FileHandle);
            }
          } break;
        }
        
        if(FileCount==FileMaxCount) break;
      }
    }
    
    FAT_FileType=FAT_FindNextFile(fn);
  }
  
  extern void FileListSort(void);
  FileListSort();
}

void FileSys_ChangePath(const char *TargetPathName)
{ 
  LastPathItemName[0]=0;
  
  if((TargetPathName[0]=='.')&&(TargetPathName[1]=='.')){ 
    u32 FlashPos=(u32)-1;
    
    u32 cnt=0;
    while(FileSys_PathName[cnt]!=0){ 
      if(FileSys_PathName[cnt]=='/') FlashPos=cnt;
      cnt++;
    }
    
    if(FlashPos!=(u32)-1) strcpy(LastPathItemName,(char*)&FileSys_PathName[FlashPos+1]);
    iprintf("%d [%s] [%s]\n",FlashPos,LastPathItemName,FileSys_PathName);
  }
  
  if(TargetPathName==NULL){ 
    strcpy(FileSys_PathName,"/");
    FileSys_RefreshPath();
    return;
  }
  
  if(TargetPathName[0]==0){ 
    strcpy(FileSys_PathName,"/");
    FileSys_RefreshPath();
    return;
  }
  
  // Rootから
  if(TargetPathName[0]=='/'){ 
    strcpy(FileSys_PathName,TargetPathName);
    FileSys_RefreshPath();
    return;
  }
  
  // UpPath
  if((TargetPathName[0]=='.')&&(TargetPathName[1]=='.')){ 
    if(strlen(FileSys_PathName)<2){ 
      strcpy(FileSys_PathName,"/");
      FileSys_RefreshPath();
      return;
    }
    
    { 
      u32 FlashPos=0;
      u32 cnt=0;
      while(FileSys_PathName[cnt]!=0){ 
        if(FileSys_PathName[cnt]=='/') FlashPos=cnt;
        cnt++;
      }
      FileSys_PathName[FlashPos]=0;
      if(strlen(FileSys_PathName)<2) strcpy(FileSys_PathName,"/");
      FileSys_RefreshPath();
      return;
    }
  }
  
  if(strlen(FileSys_PathName)<2){ 
    }else{ 
    strcat(FileSys_PathName,"/");
  }
  
  strcat(FileSys_PathName,TargetPathName);
  
  FileSys_RefreshPath();
  
  return;
}

bool FileSys_isPathExists(const char *TargetPathName)
{
  if(TargetPathName==NULL) return(false);
  if(TargetPathName[0]!='/') return(false);
  if(TargetPathName[1]=='/') return(false);
  
  bool e;
  
  e=FAT_CWD(TargetPathName);
  FAT_CWD(FileSys_PathName);
  
  return(e);
}

char* FileSys_GetPathName(void)
{ 
  return(FileSys_PathName);
}

s32 FileSys_GetLastPathItemIndex(void)
{ 
  s32 idx=0;
  for(idx=0;idx<FileCount;idx++){ 
    if(pFile[idx].FileType==FT_Path){ 
      if(strcmp(pFile[idx].Alias,LastPathItemName)==0) return(idx);
    }
  }
  
  return(-1);
}

s32 FileSys_GetFileCount(void)
{ 
  return(FileCount);
}

s32 FileSys_GetPureFileCount(void)
{
  s32 cnt=0;
  s32 idx=0;

  for(idx=0;idx<FileCount;idx++){ 
    if(pFile[idx].FileType==FT_File) cnt++;
  }
  
  return(cnt);
}

char* FileSys_GetAlias(s32 FileIndex)
{ 
  if((FileIndex<0)||(FileCount<=FileIndex)){
    static char pf[2]={0,};
    return(pf);
  }
  return(pFile[FileIndex].Alias);
}

UnicodeChar* FileSys_GetFilename(s32 FileIndex)
{ 
  if((FileIndex<0)||(FileCount<=FileIndex)){
    static UnicodeChar pf[2]={0,};
    return(pf);
  }
  return(pFile[FileIndex].Filename);
}

u32 FileSys_GetFileType(s32 FileIndex)
{ 
  return(pFile[FileIndex].FileType);
}

int FileSys_GetFileTrackNum(s32 FileIndex)
{ 
  return(pFile[FileIndex].TrackNum);
}

void FileSys_GetFileExt(s32 FileIndex,char *ext)
{ 
  char *Alias=FileSys_GetAlias(FileIndex);
  
  u32 cnt=0;
  
  u32 DotPos=0;
  cnt=0;
  while(Alias[cnt]!=0){ 
    if(Alias[cnt]=='.') DotPos=cnt;
    cnt++;
  }
  
  ext[0]=0;
  ext[1]=0;
  ext[2]=0;
  ext[3]=0;
  ext[4]=0;
  
  if(DotPos!=0) strncpy(ext,&Alias[DotPos],4);
  
  cnt=0;
  while(ext[cnt]!=0){ 
    char c=ext[cnt];
    
    if ((c>0x60)&&(c<0x7B)){ 
      c=c-0x20;
      ext[cnt]=c;
    }
    cnt++;
  }
}

u32 FileSys_GetFileDataSize(u32 FileIndex)
{ 
  return(pFile[FileIndex].FileSize);
}

bool FileSys_GetFileData(u32 FileIndex,u8 *dstbuf)
{ 
  int fh;
  
  fh=FileSys_fopen(FileIndex);
  if(fh==0) return(false);
  
  FileSys_fread(dstbuf,1,FileSys_GetFileDataSize(FileIndex),fh);
  FileSys_fclose(fh);
  
  return(true);
}

#define MemFileCount (8)

typedef struct {
  bool Enabled;
  FAT_FILE *file;
} TMemFile;

static TMemFile MemFile[MemFileCount];

FAT_FILE *FileSys_Get_GBANDSFAT_FileHandle(int hFile)
{ 
  TMemFile *pMemFile=&MemFile[hFile];
  return(pMemFile->file);
}

static inline int GetFreeMemFileIndex(void)
{
  int idx=1;
  for(idx=1;idx<MemFileCount;idx++){
    if(MemFile[idx].Enabled==false) return(idx);
  }
  
  return(0);
}

int FileSys_fopen_DirectMapping(FAT_FILE *file)
{ 
  if(file==NULL){ 
    iprintf("fopen Error:file handle is NULL\n");
    return(0);
  }
  
  int hFile=GetFreeMemFileIndex();
  
  if(hFile==0){ 
    iprintf("fopen Error:Already Opened.\n");
    return(0);
  }
  
  TMemFile *pMemFile=&MemFile[hFile];
  
  pMemFile->file=file;
  pMemFile->Enabled=true;
  
  return(hFile);
}

int FileSys_fopen(u32 FileIndex)
{ 
  int hFile=GetFreeMemFileIndex();
  if(hFile==0){ 
    iprintf("fopen Error:Already Opened.\n");
    return(0);
  }
  
  const char *pFilename=NULL;
  
  switch(FileIndex){
    case SystemFileID_Thumbnail1: pFilename=&SystemFilename_Thumbnail1[0]; break;
    default: pFilename=pFile[FileIndex].Alias; break;
  }
  
  FAT_FILE *file=FAT_fopen(pFilename,"r");
  if(file==NULL) return(0);
  
  TMemFile *pMemFile=&MemFile[hFile];
  
  pMemFile->file=file;
  pMemFile->Enabled=true;
  
  return(hFile);
}

bool FileSys_fclose (int hFile)
{ 
  TMemFile *pMemFile=&MemFile[hFile];
  if(pMemFile->Enabled==false) return(false);
  
  pMemFile->Enabled=false;
  
  if(pMemFile->file!=NULL) return(FAT_fclose(pMemFile->file));
  
  return(false);
}

long int FileSys_ftell (int hFile)
{ 
  TMemFile *pMemFile=&MemFile[hFile];
  if(pMemFile->Enabled==false) return(0);
  
  return(FAT_ftell(pMemFile->file));
}

int FileSys_fseek(int hFile, u32 offset, int origin)
{ 
  TMemFile *pMemFile=&MemFile[hFile];
  if(pMemFile->Enabled==false) return(0);
  
  return(FAT_fseek(pMemFile->file,offset,origin));
}

u32 FileSys_fread (void* buffer, u32 size, u32 count, int hFile)
{ 
  TMemFile *pMemFile=&MemFile[hFile];
  if(pMemFile->Enabled==false) return(0);
  
  return(FAT_fread(buffer,size,count,pMemFile->file));
}

u32 FileSys_fread_fast (void* buffer, u32 size, u32 count, int hFile)
{ 
  TMemFile *pMemFile=&MemFile[hFile];
  if(pMemFile->Enabled==false) return(0);
  
  return(FAT_fread_fast16bit(buffer,size,count,pMemFile->file));
}

// ---------------------------------------------------

inline static bool isSwapFile(TFile *pf0,TFile *pf1)
{
  UnicodeChar *puc0=&pf0->Filename[0];
  UnicodeChar *puc1=&pf1->Filename[0];
  
  while(1){
    u32 uc0=*puc0;
    u32 uc1=*puc1;
    if(((u32)'A'<=uc0)&&(uc0<=(u32)'Z')) uc0+=0x20;
    if(((u32)'A'<=uc1)&&(uc1<=(u32)'Z')) uc1+=0x20;
    
    if(uc0==uc1){
      // 同一フ?イル名はないはず…だったのにね。
      if((uc0==0)&&(uc1==0)){
        if(pf0->TrackNum<=pf1->TrackNum){
          return(false);
          }else{
          return(true);
        }
      }
      }else{
      // フ?イル名長さ?ェック
      if(uc0==0) return(false);
      if(uc1==0) return(true);
      // 文字比較
      if(uc0<uc1) return(false);
      if(uc0>uc1) return(true);
    }
    
    puc0++; puc1++;
  }
  return(false);
}

void FileListSort(void)
{
  if(FileCount<2) return;
  
  static u32 topdata[1024]; // FileMaxCount];
  
  int idx=0;
  int idx0=0;
  int idx1=0;

  for(idx=0;idx<FileCount;idx++){
    u32 uc0=(u32)pFile[idx].Filename[0];
    u32 uc1=(u32)pFile[idx].Filename[1];
    if(((u32)'A'<=uc0)&&(uc0<=(u32)'Z')) uc0+=0x20;
    if(((u32)'A'<=uc1)&&(uc1<=(u32)'Z')) uc1+=0x20;
    topdata[idx]=(uc0<<16) | uc1;
  }
  
  for(idx0=0;idx0<FileCount-1;idx0++){
    for(idx1=idx0+1;idx1<FileCount;idx1++){
      TFile *pf0=&pFile[idx0];
      TFile *pf1=&pFile[idx1];
      bool SwapFlag=false;
      
      if(pf0->FileType!=pf1->FileType){
        if(pf0->FileType==FT_File) SwapFlag=true;
        }else{
        if(topdata[idx0]>=topdata[idx1]){
          if(isSwapFile(pf0,pf1)==true) SwapFlag=true;
        }
      }
      
      if(SwapFlag==true){
        TFile ftemp=*pf0;
        *pf0=*pf1;
        *pf1=ftemp;
        
        u32 tmp=topdata[idx0];
        topdata[idx0]=topdata[idx1];
        topdata[idx1]=tmp;
      }
    }
  }
  
}

// ------------------------------------------------------------------------

static void Shuffle_Refresh(int TopIndex)
{
  ShuffleCount=FileCount;
  int idx=1;
  pShuffleList[0]=TopIndex;

  for(idx=1;idx<ShuffleCount;idx++){
    pShuffleList[idx]=-1;
  }
  
  for(idx=0;idx<ShuffleCount;idx++){
    if(idx!=TopIndex){
      int r=(rand()%ShuffleCount)+1;
      int fidx=0;
      while(r!=0){
        fidx=(fidx+1)%ShuffleCount;
        if(pShuffleList[fidx]==-1) r--;
      }
//      iprintf("ref%d,%d\n",fidx,idx);
      pShuffleList[fidx]=idx;
    }
  }
  
}

void Shuffle_Clear(void)
{
  ShuffleCount=-1;
}

int Shuffle_GetNextIndex(int LastIndex)
{
  if(ShuffleCount!=FileCount) Shuffle_Refresh(LastIndex);
  int idx=0;
  for(idx=0;idx<ShuffleCount;idx++){
    if(pShuffleList[idx]==LastIndex){
      if(idx==(ShuffleCount-1)){
        ShuffleCount=-1;
        return(-1);
        }else{
        return(pShuffleList[idx+1]);
      }
    }
  }
  
  iprintf("Shuffle_GetNextIndex(%d):not found next index?\n",LastIndex);
  for(idx=0;idx<ShuffleCount;idx++){
    iprintf("%d,",pShuffleList[idx]);
  }
 
  return(-1);
}

int Shuffle_GetPrevIndex(int LastIndex)
{
  if(ShuffleCount!=FileCount) Shuffle_Refresh(LastIndex);
  
  int idx=0;
  for(idx=0;idx<ShuffleCount;idx++){
    if(pShuffleList[idx]==LastIndex){
      if(idx==0){
        ShuffleCount=-1;
        return(-1);
        }else{
        return(pShuffleList[idx-1]);
      }
    }
  }
  
  iprintf("Shuffle_GetNextIndex(%d):not found next index?\n",LastIndex);
  for(idx=0;idx<ShuffleCount;idx++){
    iprintf("%d,",pShuffleList[idx]);
  }
  
  return(-1);
}

int Normal_GetNextIndex(int LastIndex)
{
  LastIndex++;
  if(LastIndex==FileCount) return(-1);
  
  return(LastIndex);
}

int Normal_GetPrevIndex(int LastIndex)
{
  if(LastIndex==0) return(-1);
  
  LastIndex--;
  
  return(LastIndex);
}

void SetARM9_REG_WaitCR(void)
{
  #define _REG_WAIT_CR (*(vuint16*)0x04000204)

  u32 SetARM9_REG_ROM1stAccessCycleControl;
  u32 SetARM9_REG_ROM2stAccessCycleControl;
  u16 bw=_REG_WAIT_CR;
  
  bw&=BIT8 | BIT9 | BIT10 | BIT12 | BIT13;
  
  // mp2 def.0x6800
  // loader def.0x6000
  
  bw|=2 << 0; // 0-1  RAM-region access cycle control   0..3=10,8,6,18 cycles def.0
  bw|=(u16)SetARM9_REG_ROM1stAccessCycleControl << 2; // 2-3  ROM 1st access cycle control   0..3=10,8,6,18 cycles def.0
  bw|=(u16)SetARM9_REG_ROM2stAccessCycleControl << 4; // 4    ROM 2nd access cycle control   0..1=6,4 cycles def.0
  bw|=0 << 5; // 5-6  PHI-terminal output control   0..3=Lowlevel, 4.19MHz, 8.38MHZ, 16.76MHz clock output def.0
  bw|=0 << 7; // 7    Cartridge access right   0=ARM9, 1=ARM7 def.0
  bw|=0 << 11; // 11   Card access right   0=ARM9, 1=ARM7 def.1
  bw|=1 << 14; // 14   Main Memory Interface mode   0=Asychronous (prohibited!), 1=Synchronous def.1
  bw|=1 << 15; // 15   Main Memory priority   0=ARM9 priority, 1=ARM7 priority def.0
  
  //bw|=1 << 7; // 7    Cartridge access right   0=ARM9, 1=ARM7 def.0
  
  _REG_WAIT_CR=bw;

  #undef _REG_WAIT_CR
}