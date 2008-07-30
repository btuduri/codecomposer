#include <nds.h>
#include <nds/arm9/console.h>
#include <stdio.h>
#include <string.h>

#include "std.h"
#include "inifile.h"
#include "smidlib.h"
#include "smidlib_pch.h"
#include "smidlib_sm.h"
#include "smidlib_mtrk.h"
#include "setarm9_reg_waitcr.h"
#include "_const.h"
#include "memtool.h"
#include "filesys.h"
#include "plugin_supple.h"
#include "shell.h"
#include "cimfs.h"
#include "directdisk.h"
#include "../../ipc3.h"
#include "strpcm.h"
#include "extmem.h"

#include "smidproc.h"

// files from DSMI
#include "keyboard.raw.h"
#include "keyboard.pal.h"
#include "keyboard.map.h"
#include "keyb_hit.h"
#include "bg_main.h"
#include "bg_sub.h"
#include "font_8x11.h"
#include "fontchars.h"

extern u8 *DeflateBuf;
extern u32 DeflateSize;
extern u32 SamplePerFrame;
extern int TotalClock,CurrentClock;
extern u32 ClockCur;
extern ALIGNED_VAR_IN_DTCM s32 DTCM_tmpbuf[MaxSamplePerFrame*2];
extern ALIGNED_VAR_IN_DTCM s32 DTCM_dstbuf[MaxSamplePerFrame*2];
extern u32 SetARM9_REG_ROM1stAccessCycleControl;
extern u32 SetARM9_REG_ROM2stAccessCycleControl;
extern u32 StackIndex;
extern u32 StackCount;
extern s32 *pStackBuf[ChannelsCount][MaxStackCount];
extern s32 StackLastCount[ChannelsCount];
extern void *pReserve[ReserveCount];
extern u32 SoundFontLoadTimeus;
extern TPluginBody *pPB;
extern TiniMIDPlugin *MIDPlugin;
extern s16 *strpcmRingLBuf;
extern s16 *strpcmRingRBuf;

// macros and variables from DSMI
#define PEN_DOWN (~IPC->buttons & (1 << 6))
#define TICKS_PER_ROW	200
#define N_ROWS		28

u16 row = 0;
u16 tick = 0;
u8 notes[] = {2, 255, 5, 255, 7, 255, 255, 2, 255, 5, 255, 8, 7, 255, 255, 255, 2, 255, 5, 255, 7, 255, 255, 5, 255, 2, 255, 255};
u8 keyb_xpos = 2, keyb_ypos = 10, keyb_width = 28, keyb_height = 5;
uint16* map = (uint16*)SCREEN_BASE_BLOCK(8);
u8 lastnote = 0;
touchPosition touch;
int touch_was_down = 0;
u8 halftones[] = {1, 3, 6, 8, 10, 13, 15, 18, 20, 22};
u8 channel = 0;
u8 baseOctave = 2;
u8 pm_x0 = 0, pm_y0 = 0;
u16 currnote = 0;
u16 *main_vram = (u16*)BG_BMP_RAM(2);

// added by KHS
int flagLoadProgram = 0;

// declaration from DSMI
void play(u8 note);
void stop(u8 note);
void pitchChange(s16 value);
void pressureChange(u8 value);
u8 isHalfTone(u8 note);
void setKeyPal(u8 note);
void resetPals(void);
void Smoke(void);
void drawFullBox(u8 tx, u8 ty, u8 tw, u8 th, u16 col);
void drawString(const char* str, u8 tx, u8 ty);
void displayOctave(u8 chn);
void displayChannel(u8 chn);
void VblankHandler();

// refer to this site: http://dldi.drunkencoders.com/index.php?title=GBA_NDS_FAT
int main(void)
{	
	InitInterrupts();

	videoSetMode(0);	//not using the main screen
	videoSetModeSub(MODE_0_2D | DISPLAY_BG0_ACTIVE);	//sub bg 0 will be used to print text
	vramSetBankC(VRAM_C_SUB_BG);

	SUB_BG0_CR = BG_MAP_BASE(31);

	BG_PALETTE_SUB[255] = RGB15(31,31,31);	//by default font will be rendered with color 255

	//consoleInit() is a lot more flexible but this gets you up and running quick
	consoleInitDefault((u16*)SCREEN_BASE_BLOCK_SUB(31), (u16*)CHAR_BASE_BLOCK_SUB(0), 16);

	strpcmSetVolume16(16);
  
	IPC3->strpcmLBuf=NULL;
	IPC3->strpcmRBuf=NULL;

    pIMFS = new CIMFS();
    if(pIMFS->InitIMFS()==false) 
		while(1);

    FileSys_Init(256);
    Shell_AutoDetect();
  
    if(Shell_FindShellPath()==false) 
		while(1);
	
	strpcmSetVolume16(16);
	DD_Init(EDDST_FAT);
	
	flagLoadProgram = 0;
	iprintf("main: flagLoadProgram is %d\n", flagLoadProgram);

    // extmem_Init();

	// DSMI의 UI는 일단 막아놨습니다, 하지만 그래도 터치스크린 작동은 되므로 테스트할 수 있습니다.
	// 주석만 풀면 바로 사용이 가능합니다.
	/*
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	lcdMainOnBottom();
	
	// Set modes
	videoSetMode(MODE_5_2D | DISPLAY_BG0_ACTIVE | DISPLAY_BG2_ACTIVE);
	videoSetModeSub(MODE_5_2D | DISPLAY_BG0_ACTIVE | DISPLAY_BG3_ACTIVE);
	
	// Set banks
	vramSetMainBanks(VRAM_A_MAIN_BG_0x06000000, VRAM_B_MAIN_BG_0x06020000, VRAM_C_SUB_BG_0x06200000 , VRAM_D_LCD);
	
	// sub display
	SUB_BG3_CR = BG_BMP16_256x256  | BG_BMP_BASE(2);
	SUB_BG3_XDX = 1 << 8;
	SUB_BG3_XDY = 0;
	SUB_BG3_YDX = 0;
	SUB_BG3_YDY = 1 << 8;
	
	// Text bg on sub
	SUB_BG0_CR = BG_MAP_BASE(4) | BG_TILE_BASE(0) | BG_PRIORITY(0);
	BG_PALETTE_SUB[255] = RGB15(31,31,31);
	consoleInitDefault((u16*)SCREEN_BASE_BLOCK_SUB(31), (u16*)CHAR_BASE_BLOCK_SUB(0), 16);

	// The main display is for graphics.
	// Set up an extended rotation background for background gfx
	BG2_CR = BG_BMP16_256x256 | BG_BMP_BASE(2);
	BG2_XDX = 1 << 8;
	BG2_XDY = 0;
	BG2_YDX = 0;
	BG2_YDY = 1 << 8;
	
	// Set up tile mode for the keyboard
	BG0_CR = BG_COLOR_16 | BG_32x32 | BG_MAP_BASE(8) | BG_TILE_BASE(0);
	
	// Clear tile mem
	u32 i;
	for(i=0; i<(32*1024); ++i) {
		((u16*)BG_BMP_RAM(0))[i] = 0;
	}
	
	// Copy tiles and palettes
	dmaCopy((uint16*)keyboard_Palette, (uint16*)BG_PALETTE, 32);
	dmaCopy((uint16*)keyboard_fullnotehighlight_Palette, (uint16*)BG_PALETTE+16, 32);
	dmaCopy((uint16*)keyboard_halfnotehighlight_Palette, (uint16*)BG_PALETTE+32, 32);
	dmaCopy((uint16*)keyboard_Tiles, (uint16*)CHAR_BASE_BLOCK(0), 736);
	
	// Fill screen with empty tiles
	for(i=0;i<768;++i)
		map[i] = 28;
	
	// Draw the backgrounds
	for(i=0; i<192*256; ++i) {
		((uint16*)BG_BMP_RAM(2))[i] = ((uint16*)bg_main)[i];
	}
	
	for(i=0; i<192*256; ++i) {
		((uint16*)BG_BMP_RAM_SUB(2))[i] = ((uint16*)bg_sub)[i];
	}

	displayChannel(channel);
	displayOctave(baseOctave);

	u8 x, y;
	for(y=0; y<5; ++y) {
		for(x=0; x<28; ++x) {
			map[32*(y+keyb_ypos)+(x+keyb_xpos)] = keyboard_Map[29*y+x+1];
		}
	}
	*/

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	if(!initSmidlib())
	{
		iprintf("failed to initialize midi sound system\n");
		return false;
	}

	/*
	int file_handle = 0;
	file_handle = Shell_OpenFile("Dancing_Queen.mid");
	
	if(Start(file_handle) == false)
	{
      iprintf("midi plugin start error.\n");
      return 0;
    }
	*/

	EstrpcmFormat SPF = GetOversamplingFactorFromSampleRate(MIDPlugin->SampleRate);
	strpcmStart(false, MIDPlugin->SampleRate, SamplePerFrame, 2, SPF);
	
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	while(1)
	{
		VblankHandler();
		swiWaitForVBlank();
	}
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////

	return 0;
}


void InitInterrupts(void)
{
  REG_IME = 0;
  
  irqInit();
  
  irqEnable(IRQ_VBLANK);
  irqEnable(IRQ_IPC_SYNC);
  irqEnable(IRQ_TIMER3);

  irqSet(IRQ_IPC_SYNC, InterruptHandler_IPC_SYNC);
  irqSet(IRQ_TIMER3, Smoke);

  REG_IPC_SYNC=IPC_SYNC_IRQ_ENABLE;

  REG_IME = 1;
}


// functions from DSMI
void play(u8 note)
{
	TStdMIDI *_StdMIDI=&StdMIDI;
	u32 value1 = (u32)channel;
	u32 value2 = (u32)note+12* (u32)baseOctave;
	u32 value3 = 127;

	iprintf("channel is %d, baseOctave is %d\n", channel, baseOctave);
	iprintf("NoteOn: v1: %d, v2: %d, v3 :%d\n", value1, value2, value3);
	
	if(flagLoadProgram == 0)
	{
		// iprintf("play: flagLoadProgram is false\n");
		flagLoadProgram = 1;
		MTRK_NoteOn_LoadProgram(value1, value2, value3);
	}
	else
	{
		// iprintf("play: flagLoadProgram is true\n");
		MTRK_NoteOn(value1, 0, value2, value3);
	}

    _StdMIDI->FastNoteOn=true;
}

void stop(u8 note)
{
	// u32 value1 = 0x90 | (u32)channel;
	u32 value1 = (u32)channel;
	u32 value2 = (u32)note+12* (u32)baseOctave;
	u32 value3 = 0;

	iprintf("channel is %d, baseOctave is %d\n", channel, baseOctave);
	iprintf("NoteOff: v1: %d, v2: %d, v3 :%d\n", value1, value2, value3);

	if(flagLoadProgram = true)
		MTRK_NoteOff(value1, value2, value3);
}

void pitchChange(s16 value)
{
	s16 newvalue = value;
	
	// Clamp to [-64, 64]
	if(newvalue < -64) newvalue = -64;
	if(newvalue > 63) newvalue = 63;
	
	// Make positive
	newvalue += 64;
	
	if(flagLoadProgram == 1)
	{
	  iprintf("pitchChange value is %d\n", newvalue);
	  MTRK_ChangePitchBend((u32)channel, (s32)newvalue);
	}
}

void pressureChange(u8 value)
{
	u8 newvalue = value;
	
	// Clamp
	if(newvalue > 127) newvalue = 127;
	
	if(flagLoadProgram == 1)
	{	
		iprintf("pressureChange value is %d\n", newvalue);
		MTRKCC_Proc((u32)channel, 0, newvalue);
	}
}

// 1 for halftones, 0 for fulltones
u8 isHalfTone(u8 note)
{
	u8 i;
	for(i=0;i<10;++i) {
		if(note==halftones[i])
			return 1;
	}
	return 0;
}

// Set the key corresp. to note to use the palette corresp. to pal_idx
void setKeyPal(u8 note)
{
	u8 x, y, hit_row, pal_idx;

	if(isHalfTone(note)) {
		hit_row = 0;
		pal_idx = 2;
	} else {
		hit_row = 4;
		pal_idx = 1;
	}

	for(x=0; x<28; ++x) {
		if(keyb_hit[hit_row][x] == note) {
			for(y=0; y<5; ++y) {
				map[32*(y+keyb_ypos)+(x+keyb_xpos)] &= ~(3 << 12); // Clear bits 12 and 13 (from the left)
				map[32*(y+keyb_ypos)+(x+keyb_xpos)] |= (pal_idx << 12); // Write the pal index to bits 12 and 13
			}
		}
	}
}

// Reset keyboard colors to normal
void resetPals(void)
{
	u8 x,y;
	for(x=0; x<28; ++x) {
		for(y=0; y<5; ++y) {
			map[32*(y+keyb_ypos)+(x+keyb_xpos)] &= ~(3 << 12); // Clear bits 12 and 13 (from the left)
		}
	}
}

void Smoke(void)
{
	if(tick==0) {
		if(notes[row]!=255) {
			play(notes[row]);
		}
		row++;
	}
	if(row==N_ROWS) {
		TIMER3_CR = 0;
		irqDisable(IRQ_TIMER3);
	}

	tick++;
	tick%=TICKS_PER_ROW;
}

void drawFullBox(u8 tx, u8 ty, u8 tw, u8 th, u16 col)
{
	u8 i,j;
	for(i=0;i<tw;++i) {
		for(j=0;j<th;++j) {
			*(main_vram+SCREEN_WIDTH*(ty+j)+i+tx) = col;
		}
	}
}

void drawString(const char* str, u8 tx, u8 ty)
{
	// Draw text
	u8 pos=0, charidx, i, j;
	u16 drawpos = 0; u8 col;
	char *charptr;
	
	while(pos<strlen(str))
	{
		charptr = strchr(fontchars, str[pos]);
		if(charptr==0) {
			charidx = 39; // '_'
		} else {
			charidx = charptr - fontchars;
		}
		
		for(j=0;j<11;++j) {
			for(i=0;i<8;++i) {
				// Print a character from the bitmap font
				// each char is 8 pixels wide, and 8 pixels
				// are in a byte.
				col = font_8x11[64*j+charidx];
				if(col & BIT(i)) {
					*(main_vram+SCREEN_WIDTH*(j+ty)+(i+tx+drawpos)) = RGB15(0,0,0) | BIT(15);
				}
			}
		}
		
		drawpos += charwidths_8x11[charidx]+1;
		pos++;
	}
}

void displayOctave(u8 chn)
{
	char cstr[3] = {0, 0, 0};
	sprintf(cstr, "%u", chn);
	drawFullBox(102, 155, 14, 9, RGB15(26, 26, 26) | BIT(15));
	drawString(cstr, 102, 155);
}

void displayChannel(u8 chn)
{
	char cstr[3] = {0, 0, 0};
	sprintf(cstr, "%u", chn);
	drawFullBox(206, 155, 14, 9, RGB15(26, 26, 26) | BIT(15));
	drawString(cstr, 206, 155);
}

void VblankHandler()
{
	scanKeys();
	touch=touchReadXY();
	
	if(!touch_was_down && PEN_DOWN)
	{
		iprintf("case #1\n");
		touch_was_down = 1;
		
		pm_x0 = touch.px;
		pm_y0 = touch.py;
		
		// Is the pen on the keyboard?
		if(     (touch.px > 8*keyb_xpos) && (touch.py > 8*keyb_ypos)
			&& (touch.px < 8*(keyb_xpos+keyb_width)) && (touch.py < 8*(keyb_ypos+keyb_height)) ) {
			
			// Look up the note in the hit-array
			u8 kbx, kby;
			kbx = touch.px/8 - keyb_xpos;
			kby = touch.py/8 - keyb_ypos;

			u8 note = keyb_hit[kby][kbx];
			currnote = note + 12*baseOctave;
			lastnote = note;
			setKeyPal(note);
			
			// Play the note
			play(note);
			strpcmUpdate_mainloop();
		}
	}
	else if(touch_was_down && !PEN_DOWN)
	{
		iprintf("case #2\n");
		touch_was_down = 0;
		resetPals();
		stop(lastnote);
		strpcmUpdate_mainloop();

		pitchChange(0);
		strpcmUpdate_mainloop();
	}
	
	if(touch_was_down && PEN_DOWN)
	{
		iprintf("case #3\n");
		s16 dx, dy;
		dx = touch.px - pm_x0;
		if(dx<0) dx = 0;
		dy = touch.py - pm_y0;

		pitchChange(-dy);
		strpcmUpdate_mainloop();

		pressureChange(dx);
		strpcmUpdate_mainloop();
	}
	
	u16 keys = keysDown();
	
	if(keys & KEY_X) {
		pitchChange(0);
		strpcmUpdate_mainloop();
	}
	
	if(keys & KEY_Y) {
		pressureChange(0);
		strpcmUpdate_mainloop();
	}
	
	if(keys & KEY_RIGHT)
	{
		if(baseOctave < 9)
			baseOctave++;
		
		displayOctave(baseOctave);
		iprintf("base octave %u\n", baseOctave);
	}
	
	if(keys & KEY_LEFT)
	{
		if(baseOctave > 0)
			baseOctave--;
		
		displayOctave(baseOctave);
		iprintf("base octave %u\n", baseOctave);
	}
	
	if(keys & KEY_UP)
	{
		if(channel < 15)
			channel++;
		
		displayChannel(channel);
		iprintf("using midi channel %u\n", channel);
	}
	
	if(keys & KEY_DOWN)
	{
		if(channel > 0)
			channel--;
		
		displayChannel(channel);
		iprintf("using midi channel %u\n", channel);
	}
	
	if(keys & KEY_B) {
		TIMER3_DATA = TIMER_FREQ_64(1000);
		TIMER3_CR = TIMER_ENABLE | TIMER_DIV_64 | TIMER_IRQ_REQ;
		row=0;
		tick=0;
	}
	
	keys = keysUp();
}