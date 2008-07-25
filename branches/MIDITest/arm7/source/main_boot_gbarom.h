
/*
 referrence from 2006-01-13 - v2.11
 
  NDS MP
 GBAMP NDS Firmware Hack Version 2.0
 An NDS aware firmware patch for the GBA Movie Player.
 By Michael Chisholm (Chishm)
 
 Large parts are based on MultiNDS loader by Darkain.
 Filesystem code based on gbamp_cf.c by Chishm (me).
 Flashing tool written by DarkFader.
 Chunks of firmware removed with help from Dwedit.

 GBAMP firmware flasher written by DarkFader.
 
 This software is completely free. No warranty is provided.
 If you use it, please give due credit and email me about your
 project at chishm@hotmail.com
*/

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Firmware stuff

#define FW_READ        0x03

__attribute__((noinline)) static void _readFirmware(uint32 address, uint32 size, uint8 * buffer) {
  uint32 index;

  // Read command
  while (REG_SPICNT & SPI_BUSY);
  REG_SPICNT = SPI_ENABLE | SPI_CONTINUOUS | SPI_DEVICE_NVRAM;
  REG_SPIDATA = FW_READ;
  while (REG_SPICNT & SPI_BUSY);

  // Set the address
  REG_SPIDATA =  (address>>16) & 0xFF;
  while (REG_SPICNT & SPI_BUSY);
  REG_SPIDATA =  (address>>8) & 0xFF;
  while (REG_SPICNT & SPI_BUSY);
  REG_SPIDATA =  (address) & 0xFF;
  while (REG_SPICNT & SPI_BUSY);

  for (index = 0; index < size; index++) {
    REG_SPIDATA = 0;
    while (REG_SPICNT & SPI_BUSY);
    buffer[index] = REG_SPIDATA & 0xFF;
  }
  REG_SPICNT = 0;
}

/*-------------------------------------------------------------------------
resetMemory_ARM7
Clears all of the NDS's RAM that is visible to the ARM7
Written by Darkain.
Modified by Chishm:
 * Added STMIA clear mem loop
--------------------------------------------------------------------------*/
__attribute__((noinline)) static void resetMemory_ARM7 (bool ClearEWRAM)
{
	u32 i;
	u8 settings1, settings2;
	
	REG_IME = 0;

	for (i=0x04000400; i<0x04000500; i+=4) {
	  *((u32*)i)=0;
	}
	SOUND_CR = 0;

  for(i=0x040000B0;i<(0x040000B0+0x30);i+=4){
    *((vu32*)i)=0;
  }

  for(i=0x04000100;i<0x04000110;i+=2){
    *((u16*)i)=0;
  }

  __asm volatile (
	  "mov r0, #0x1F 				\n"
      "msr cpsr, r0					\n"
  );
  /*
  { //switch to user mode
    u32 r0=0;
    __asm {
  mov r0, #0x1F
      msr cpsr, r0    
    }
  }
  */
#if 0
  __asm volatile (
	// clear exclusive IWRAM
	// 0380:0000 to 0380:FFFF, total 64KiB
	"mov r0, #0 				\n"	
	"mov r1, #0 				\n"
	"mov r2, #0 				\n"
	"mov r3, #0 				\n"
	"mov r4, #0 				\n"
	"mov r5, #0 				\n"
	"mov r6, #0 				\n"
	"mov r7, #0 				\n"
	"mov r8, #0x03800000		\n"	// Start address 
	"mov r9, #0x03800000		\n" // End address part 1
	"orr r9, r9, #0x10000		\n" // End address part 2
	"clear_EIWRAM_loop:			\n"
	"stmia r8!, {r0, r1, r2, r3, r4, r5, r6, r7} \n"
	"cmp r8, r9					\n"
	"blt clear_EIWRAM_loop		\n"
	:
	:
	: "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7", "r8", "r9"
	);
#endif

	if(ClearEWRAM==true){
	  // clear most of EWRAM - except after 0x023FF800, which has DS settings
	  u32 *startadr=(u32*)0x02008000;
	  u32 *endadr=  (u32*)0x023ff800;
	  while(startadr!=endadr){
	    *startadr++=0;
	  }
	}
  
	REG_IE = 0;
	REG_IF = ~0;
	(*(vu32*)(0x04000000-4)) = 0;  //IRQ_HANDLER ARM7 version
	(*(vu32*)(0x04000000-8)) = ~0; //VBLANK_INTR_WAIT_FLAGS, ARM7 version
	
	// POWER_CR = 1;  //turn off power to stuffs
	REG_POWERCNT = 1;
	
	// Reload DS Firmware settings
	_readFirmware((u32)0x03FE70, 0x1, &settings1);
	_readFirmware((u32)0x03FF70, 0x1, &settings2);
	
	if (settings1 > settings2) {
		_readFirmware((u32)0x03FE00, 0x70, (u8*)0x027FFC80);
	} else {
		_readFirmware((u32)0x03FF00, 0x70, (u8*)0x027FFC80);
	}
}

__attribute__((noinline)) static void boot_GBAROM(u32 BootAddress)
{
		REG_IME = IME_DISABLE;	// Disable interrupts
		REG_IF = REG_IF;	// Acknowledge interrupt
		
		if(BootAddress!=0){
			resetMemory_ARM7(true);
			*((vu32*)0x027FFE34) = BootAddress;	// Bootloader start address
			}else{
			while(IPC3->RESET_BootAddress==0){
			  vu32 w;
			  for(w=0;w<0x100;w++){
			  }
			}
			resetMemory_ARM7(false);
			*((vu32*)0x027FFE34) = IPC3->RESET_BootAddress;	// Bootloader start address
		}
		
		swiSoftReset();	// Jump to boot loader
}