#include <stdio.h>
#include <stdlib.h>
#include <NDS.h>
#include "memtoolARM7.h"
#include "a7sleep.h"

bool MappedVRAM;

#define VRAMTopAddress_Start (0x06000000)
#define VRAMTopAddress_End (VRAMTopAddress_Start+(128*1024))
u32 VRAMTopAddress;

void SetMemoryMode(bool _MappedVRAM)
{
  iprintf("SetMemoryMode(%d);\n",_MappedVRAM);
  MappedVRAM=_MappedVRAM;
  VRAMTopAddress=VRAMTopAddress_Start;
}

void SetMemoryMode_End(void)
{
  iprintf("SetMemoryMode_End();\n");
  MappedVRAM=false;
}

static void* safemalloc_VRAM(int size)
{
  void *ptr=(void*)VRAMTopAddress;
  VRAMTopAddress+=size;
  if(VRAMTopAddress_End<=VRAMTopAddress){
    iprintf("Memory overflow.\n");
    while(1);
  }
  
  static vu32 v=0;
  
  while((DMA3_CR&DMA_BUSY)!=0);
  DMA3_SRC = (u32)&v;
  DMA3_DEST = (u32)ptr;
  DMA3_CR=(DMA_32_BIT | DMA_ENABLE | DMA_START_NOW | DMA_SRC_FIX | DMA_DST_INC)+(size>>2);
  while((DMA3_CR&DMA_BUSY)!=0);
  
  return(ptr);
}

void* safemalloc(int size)
{
  size=(size+3)&~3;
//  iprintf("safemalloc(%d/0x%x);\n",size,size);
  
  if(MappedVRAM==true) return(safemalloc_VRAM(size));
  
  u32 *res=(u32*)malloc(size);
  if(res==NULL){
    iprintf("Memory overflow.\n");
    while(1);
  }
  
  static vu32 v=0;
  
  while((DMA3_CR&DMA_BUSY)!=0);
  DMA3_SRC = (u32)&v;
  DMA3_DEST = (u32)res;
  DMA3_CR=(DMA_32_BIT | DMA_ENABLE | DMA_START_NOW | DMA_SRC_FIX | DMA_DST_INC)+(size>>2);
  while((DMA3_CR&DMA_BUSY)!=0);
  
  return(res);
}

static void safefree_VRAM(void *ptr)
{
  return;
}

void safefree(void *ptr)
{
//  iprintf("safefree(0x%x);\n",(u32)ptr);
  
  if(MappedVRAM==true) return(safefree_VRAM(ptr));
  if(ptr!=NULL) free(ptr);
}

void MemCopy16DMA3(void *src,void *dst,u32 len)
{
//  MemCopy16CPU(src,dst,len);return;
  while((DMA3_CR&DMA_BUSY)!=0);
  DMA3_SRC = (u32)src;
  DMA3_DEST = (u32)dst;
  DMA3_CR=(DMA_16_BIT | DMA_ENABLE | DMA_START_NOW | DMA_SRC_INC | DMA_DST_INC)+(len>>1);
  while((DMA3_CR&DMA_BUSY)!=0);
}

void MemSet16DMA3(u16 v,void *dst,u32 len)
{
//  MemSet16CPU(v,dst,len);return;

  static vu32 dmatmp;
  dmatmp=v | (((u32)v) << 16);
  
  while((DMA3_CR&DMA_BUSY)!=0);
  DMA3_SRC = (u32)&dmatmp;
  DMA3_DEST = (u32)dst;
  DMA3_CR=(DMA_16_BIT | DMA_ENABLE | DMA_START_NOW | DMA_SRC_FIX | DMA_DST_INC)+(len>>1);
  while((DMA3_CR&DMA_BUSY)!=0);
}

static bool testmalloc(int size)
{
  if(size<=0) return(false);
  
  void *ptr;
  
  ptr=malloc(size);
  
  if(ptr==NULL) return(false);
  
  free(ptr);
  
  return(true);
}

#define PrintFreeMem_Seg (1*1024)

u32 PrintFreeMem(void)
{
#ifndef ShowDebugMsg
  return(0);
#endif

  u32 FreeMemSize=0;
  
  s32 i;
  for(i=96*1024;i!=0;i-=PrintFreeMem_Seg){
    if(testmalloc(i)==true){
      FreeMemSize=i;
      break;
    }
  }
  
  iprintf("FreeMem=%dbyte    \n",FreeMemSize);
  return(FreeMemSize);
}